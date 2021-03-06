/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "VectorDataStorage.hpp"

#include "BufferedDataTask.hpp"

using namespace type_to_vector;

BufferedDataTask::BufferedDataTask(std::string const& name)
    : BufferedDataTaskBase(name)
{
}

BufferedDataTask::BufferedDataTask(std::string const& name, RTT::ExecutionEngine* engine)
    : BufferedDataTaskBase(name, engine)
{
}

BufferedDataTask::~BufferedDataTask()
{
}

void BufferedDataTask::processSample(base::Time const& timestamp, SampleData const& sample) {

    BaseTask::processSample(timestamp, sample);

    const DataInfo& di = getDataInfo(sample.mDataInfoIndex);
    VectorBuffer& buf = mBuffers.at(di.mVectorIndex);
    
    buf.newData = true;
}

bool BufferedDataTask::createDataInfo(const PortConfig& config) {

    if (!BaseTask::createDataInfo(config))
        return false;
    
    if ( mBuffers.size() <= config.vectorIdx ) mBuffers.resize(config.vectorIdx+1);

    mBuffers.at(config.vectorIdx).mDataVectorIndex = config.vectorIdx;

    return true; 
}

void BufferedDataTask::clear() {

    BaseTask::clear();
    
    Buffers::iterator it = mBuffers.begin();

    for ( ; it != mBuffers.end(); it++ )
        if ( it->debugOut )
            ports()->removePort(it->debugOut->getName());
    
    mBuffers.clear();
}

boost::int32_t BufferedDataTask::setBufferSizeFromTime(::base::Time const & delta_time, ::base::Time const & window_start, ::base::Time const & window_length)
{

    int n = (window_start.toSeconds()+window_length.toSeconds()) / delta_time.toSeconds();
    n++;
    _buffer_size.set(n);
    return n;
}

bool BufferedDataTask::getDataMatrix(int vector_idx, int from, int to, 
        base::MatrixXd& data_matrix) {

    return mBuffers.at(vector_idx).getDataMatrix(from, to, data_matrix);
}
        
bool BufferedDataTask::getDataMatrix(int vector_idx, double from_time, double to_time, 
                base::MatrixXd& data_matrix) {

    double dt = getPeriod();
    if ( dt <= 0 ) dt = 1.0;

    return mBuffers.at(vector_idx).getDataMatrix(from_time, to_time, dt, data_matrix); 
}

bool BufferedDataTask::getTimeMatrix(int vector_idx, int from, int to, 
        base::MatrixXd& time_matrix) {

    if(!_buffer_time.get())
        LOG_WARN_S<<"Timestamps are not buffered. Calling 'getTimeMatrix' without result.";

    return mBuffers.at(vector_idx).getTimeMatrix(from, to, time_matrix);
}

bool BufferedDataTask::getTimeMatrix(int vector_idx, double from_time, double to_time, 
                base::MatrixXd& time_matrix) {

    double dt = getPeriod();
    if ( dt <= 0 ) dt = 1.0;

    return mBuffers.at(vector_idx).getTimeMatrix(from_time, to_time, dt, time_matrix); 
}

double BufferedDataTask::getBufferedSamplesRatio(unsigned int index) {
    base::MatrixXd flags;
    // get the complete flags for all samples
    mBuffers.at(index).getNewSampleFlagMatrix(0,-1,flags);

    // flags.mean() is the average of new samples for all dimensions (the flag is equal per dimension)
    return 1 - flags.mean();
}
        
bool BufferedDataTask::isBufferFilled(int vector_idx) const 
    { return mBuffers.at(vector_idx).isFilled(); }


bool BufferedDataTask::isDataAvailable () const {

    if (mBuffers.empty()) return false;

    Buffers::const_iterator it = mBuffers.begin();

    for ( ; it != mBuffers.end(); it++)
        if ( !it->isFilled() ) return false;

    return true;
}

void BufferedDataTask::updateData() {

    BufferedDataTaskBase::updateData();

    base::VectorXd buffered_samples_ratio(mBuffers.size());

    Buffers::iterator it = mBuffers.begin();
    
    for ( int i=0; it != mBuffers.end(); it++,i++ ) {

        if ( ! it->isCreated() ) {
            const DataVector& dv = getDataVector(it->mDataVectorIndex);
            it->create(dv, _buffer_size.get(), _buffer_time.get());
        }

        if ( it->isCreated() && 
                ((_buffer_new_only.get() && it->newData) || !_buffer_new_only.get()) ) {

            const DataVector& dv = getDataVector(it->mDataVectorIndex);
            // push the data vector and the newData flag
            it->push(dv);

            it->newData = false;
            
            it->writeDebug();

            // store ratio per data index
            buffered_samples_ratio[it->mDataVectorIndex] = getBufferedSamplesRatio(it->mDataVectorIndex);
        }
    }

     // write ratio
    _buffered_samples_ratio.write(buffered_samples_ratio);
}

bool BufferedDataTask::configureHook() {

    if (!BufferedDataTaskBase::configureHook() )
        return false;

    // create debug ports for the buffers
    if ( _debug_buffer.get() ) {
        
        Buffers::iterator it = mBuffers.begin();
        for ( int idx=0; it != mBuffers.end(); it++,idx++) {

            if ( !it->debugOut ) {

                std::string idx_str = boost::lexical_cast<std::string>(idx);
                it->debugOut = createOutputPort("debug_buffer_" + idx_str, 
                        "/type_to_vector/BufferContent");
            }
        }
    }

    return true;
}
