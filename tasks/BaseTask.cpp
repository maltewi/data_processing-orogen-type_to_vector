/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include <rtt/base/PortInterface.hpp>
#include <rtt/base/InputPortInterface.hpp>
#include <rtt/DataFlowInterface.hpp>
#include <rtt/typelib/TypelibMarshallerBase.hpp>
#include <rtt/plugin/PluginLoader.hpp>
#include <typelib/registry.hh>

#include <type_to_vector/VectorTocMaker.hpp>
#include <type_to_vector/Converter.hpp>

#include <base/Eigen.hpp>
#include "VectorDataStorage.hpp"

#include "../TypeToVectorTypes.hpp"

#include "BaseTask.hpp"

using namespace type_to_vector;
using RTT::log;
using RTT::endlog;
using RTT::Debug;
using RTT::Info;
using RTT::Warning;
using RTT::Error;

namespace type_to_vector {
class TimeNowConversion : public AbstractConverter {

public:
    TimeNowConversion(const VectorToc& toc) : AbstractConverter(toc) {}

    VectorOfDoubles apply (void* data, bool create_places=false) {
        mVector.clear();
        mVector.push_back(base::Time::now().toSeconds());
        if (mPlaceVector.empty())
            mPlaceVector.push_back("time"); 
        return mVector;
    }
};
}

const int BaseTask::MAX_VECTOR_INDEX = 100;

BaseTask::BaseTask(std::string const& name)
    : BaseTaskBase(name), mpRegistry(new Typelib::Registry())
{
}

BaseTask::BaseTask(std::string const& name, RTT::ExecutionEngine* engine)
    : BaseTaskBase(name, engine), mpRegistry(new Typelib::Registry())
{
}

BaseTask::~BaseTask()
{
    clear();
    delete mpRegistry;
}

bool BaseTask::loadTypekit(std::string const& name)
{
    return RTT::plugin::PluginLoader::Instance()->loadLibrary(name);
}

bool BaseTask::addPort(::type_to_vector::PortConfig const & port_config)
{
    if ( port_config.vectorIdx < 0 || port_config.vectorIdx >= MAX_VECTOR_INDEX )
        throw std::out_of_range("vectorIdx for " + port_config.portname + " is out of range");

    if ( !createDataInfo(port_config) ) {
        log(Error) << "couldn't create port " << port_config.portname << endlog();
        return false;
    }

    return true;
}

void BaseTask::convertBackAndWrite(){

    for(uint i = 0; i <  mDataInfos.size(); i++){

        if(mDataInfos[i].output_data_available)
        {
            mDataInfos[i].back_converter->apply(mDataInfos[i].output_vect, mDataInfos[i].sample->getRawPointer());

            mDataInfos[i].write_port->write(mDataInfos[i].sample);

            mDataInfos[i].output_data_available = false;
        }
    }
}

void BaseTask::setOutputVector(int vector_index, const Eigen::VectorXd& data){

    int start_index = 0;
    for(uint i = 0; i < mDataInfos.size(); i++)
    {
        int data_size =  mDataInfos[i].newSample.mData.size();

        if(mDataInfos[i].mVectorIndex == vector_index)
        {
            if( mDataInfos[i].output_vect.size() != data_size)
                mDataInfos[i].output_vect.resize(data_size);

            for(int j = 0; j < data_size; j++)
                mDataInfos[i].output_vect[j] = data(j + start_index);

            mDataInfos[i].output_data_available = true;

            start_index += data_size;
        }
    }
}

bool BaseTask::addDebugOutput(DataVector& vector, int vector_idx) {

    std::string idx_str = boost::lexical_cast<std::string>(vector_idx);

    vector.debugOut = createOutputPort("debug_"+idx_str,
        "/type_to_vector/ConvertedVector");

    if (!vector.debugOut)
        return false;

    return true;
}

bool BaseTask::addRawOutput(DataVector& vector, int vector_idx) {
    std::string idx_str = boost::lexical_cast<std::string>(vector_idx);

    vector.rawOut = createOutputPort("raw_out_"+idx_str,
        "/base/VectorXd");

    return vector.rawOut != 0;
}


bool BaseTask::addInputPortDataHandling(DataInfo& di) {
    
    RTT::types::TypeInfo const* type = di.readPort->getTypeInfo();

    orogen_transports::TypelibMarshallerBase* transport = 
        dynamic_cast<orogen_transports::TypelibMarshallerBase*>(
                type->getProtocol(orogen_transports::TYPELIB_MARSHALLER_ID) );

    if (! transport) {
        log(Error) << "cannot report ports of type " << type->getTypeName() << 
            " as no typekit generated by orogen defines it" << endlog();
        return false;
    }

    mpRegistry->merge(transport->getRegistry());

    if (! mpRegistry->has(transport->getMarshallingType())) {
        log(Error) << "cannot report ports of type " << type->getTypeName() << 
            " as I can't find a typekit Typelib registry that defines it" << endlog();
        return false;
    }

   
    di.typelibMarshaller = transport;
    di.handle = di.typelibMarshaller->createSample();
    di.sample = di.typelibMarshaller->getDataSource(di.handle);

    return true;
}

void BaseTask::addConvertersToInfo(DataInfo& di, const std::string& slice) {
    
    VectorToc toc = VectorTocMaker().apply(
        *(mpRegistry->get(di.typelibMarshaller->getMarshallingType())) );

    log(Debug) << "--- created toc for " << toc.mType << ":\n";
    VectorToc::const_iterator its = toc.begin();
    for ( ; its != toc.end(); its++)
        log(Debug) << its->placeDescription << " @ " << its->position << "\n";
    log(Debug) << endlog();

    VectorToc toc_sliced = VectorTocSlicer::slice(toc, slice);
    VectorToc toc_time = VectorTocSlicer::slice(toc, _time_fields.get());

    typedef AbstractConverter::Pointer ConvertPtr;

    ConvertPtr c_sliced_ptr;
    if ( toc_sliced.isFlat() )
        c_sliced_ptr = ConvertPtr(new FlatConverter(toc_sliced));
    else {
        c_sliced_ptr = ConvertPtr(new ConvertToVector(toc_sliced, *mpRegistry));
        boost::static_pointer_cast<ConvertToVector>(c_sliced_ptr)->setSlice(slice);
    }

    ConvertPtr c_time_ptr;
    if ( toc_time.empty() ) {
        c_time_ptr = ConvertPtr(new TimeNowConversion(toc_time));
        di.hasTime = false;
    } else {
        toc_time.resize(1);
        ConvertPtr c_single_ptr(new SingleConverter(toc_time));
        c_time_ptr = ConvertPtr(new MultiplyConverter(c_single_ptr, 1.0e-6));
        di.hasTime = true;
    }
    
    di.conversions.addConverter(c_sliced_ptr);
    di.conversions.addConverter(c_time_ptr);

    //For back conversion

    typedef AbstractBackConverter::Pointer BackConvertPtr;

    if ( toc_sliced.isFlat() ){
        di.back_converter = BackConvertPtr(new FlatBackConverter(toc_sliced));
    }
    else {
        di.back_converter = BackConvertPtr(new BackConverter(toc_sliced, *mpRegistry));
        boost::static_pointer_cast<BackConverter>(di.back_converter)->setSlice(slice);
    }
}


RTT::base::OutputPortInterface* BaseTask::createOutputPort(const std::string& port_name,
        const std::string& type_name) {
    
    RTT::base::PortInterface *pi = ports()->getPort(port_name);
    if(pi)
    {
        // Port exists. Should not be there already.
        log(Info) << "Port " << port_name << " is already registered." << endlog();
        return 0;
    }

    /* Check if port type is known */
    RTT::types::TypeInfoRepository::shared_ptr ti = 
        RTT::types::TypeInfoRepository::Instance();
    RTT::types::TypeInfo* type = ti->type(type_name);
    if (!type)
    {
    	log(Error) << "Cannot find port type " << type_name << 
            " in the type info repository." << endlog();
	
        return 0;
    }
    
    /* Add output port */
    RTT::base::OutputPortInterface *out_port = type->outputPort(port_name);
    if (!out_port)
    {
        log(Error) << "An error occurred during output port generation." << endlog();
        return 0;
    }
    
    ports()->addPort(out_port->getName(), *out_port);

    log(Info) << "Added output port " << port_name << " to task." << endlog();
     
    return out_port;
}

RTT::base::InputPortInterface* BaseTask::createInputPort(
                                ::std::string const & port_name, 
                                ::std::string const & type_name )
{
    
    /* Check if port already exists (check name) */
    RTT::base::PortInterface *pi = ports()->getPort(port_name);
    if(pi)
    {
        // Port exists. Returns success.
        log(Info) << "Port " << port_name << " is already registered." << endlog();
        return 0;
    }

    /* Check if port type is known */
    RTT::types::TypeInfoRepository::shared_ptr ti = 
        RTT::types::TypeInfoRepository::Instance();
    RTT::types::TypeInfo* type = ti->type(type_name);
    if (!type)
    {
    	log(Error) << "Cannot find port type " << type_name << 
            " in the type info repository." << endlog();
	
        return 0;
    }
    
    /* Create input port */
    RTT::base::InputPortInterface *in_port = type->inputPort(port_name);
    if (!in_port)
    {
        log(Error) << "An error occurred during input port generation." << endlog();
        return 0;
    }
    
    ports()->addEventPort(in_port->getName(), *(in_port) ); 
    
    return in_port;
}


bool BaseTask::createDataInfo(const PortConfig& config) {
    
    if ( config.vectorIdx < 0 ) return false;

    DataInfo di;
    
    di.readPort = createInputPort(config.portname, config.type);
    di.rawPort = static_cast<RTT::InputPort<base::VectorXd>*>(
            createInputPort(config.portname+"_raw", "/base/VectorXd") );
    di.write_port = createOutputPort(config.portname+"_out", config.type);
    if(!di.write_port) return false;

    if ( !di.readPort || !di.rawPort || !di.write_port) return false;
    
    if ( !addInputPortDataHandling(di) ) return false;
    
    di.conversions = VectorConversion(di.readPort->getName());

    if ( mVectors.size() <= config.vectorIdx ) mVectors.resize(config.vectorIdx+1);
    
    di.mVectorIndex = config.vectorIdx;

    DataVector& dv = mVectors.at(config.vectorIdx);

    aggregator::StreamAligner::Stream<SampleData>::callback_t cb = 
        boost::bind(&BaseTask::sampleCallback,this,_1,_2);

    di.streamIndex = _stream_aligner.registerStream<SampleData>(cb, 
            0, base::Time::fromSeconds(config.period));
    di.period = base::Time::fromSeconds(config.period);
    di.useTimeNow = config.useTimeNow;

    di.pStreamAligner = &_stream_aligner;
     
    di.newSample.mDataInfoIndex = mDataInfos.size();
    di.mSampleVectorIndex = dv.addVectorPart();

    addConvertersToInfo(di, config.slice);

    mDataInfos.push_back(di);

    log(Info) << "added data info for " << mDataInfos.back().readPort->getName() <<
        " with type " << mDataInfos.back().conversions.getTypeName() << 
        " to data infos index " << mDataInfos.size()-1 << " and stream index " <<
        mDataInfos.back().streamIndex << endlog();

    return true;
}


void BaseTask::processSample(base::Time const& timestamp, SampleData const& sample) {
    
    DataInfo& di = mDataInfos.at(sample.mDataInfoIndex);
    DataVector& dv = mVectors.at(di.mVectorIndex);
            
    dv.at(di.mSampleVectorIndex) = sample;

    dv.mUpdated = true;
    dv.wroteDebug = false; 
}


void BaseTask::sampleCallback(base::Time const& timestamp, SampleData const& sample) {

   processSample(timestamp, sample);
}


const DataVector& BaseTask::getDataVector(int vector_idx) const { 
    return mVectors.at(vector_idx); 
}
        
const DataInfo& BaseTask::getDataInfo(int index) const {
    return mDataInfos.at(index);
}

int BaseTask::getDataVectorCount() const {
    return mVectors.size();
}

int BaseTask::getDataInfoCount() const {
    return mDataInfos.size();
}

void BaseTask::getVector(int vector_idx, base::VectorXd& vector) const {
    mVectors.at(vector_idx).getVector(vector);
}

void BaseTask::getTimeVector(int vector_idx, base::VectorXd& time_vector) const {
    mVectors.at(vector_idx).getTimeVector(time_vector);
}

void BaseTask::getExpandedTimeVector(int vector_idx, base::VectorXd& time_vector) const {
    mVectors.at(vector_idx).getExpandedTimeVector(time_vector);
}

void BaseTask::getPlaces(int vector_idx, StringVector& places) const {
    mVectors.at(vector_idx).getPlacesVector(places);
}

bool BaseTask::isFilled(int vector_idx) const {
    return mVectors.at(vector_idx).isFilled();
}
    

void BaseTask::clear() {

    DataInfos::iterator it = mDataInfos.begin();

    for ( ; it != mDataInfos.end(); it++) {

        it->typelibMarshaller->deleteHandle(it->handle);

        _stream_aligner.unregisterStream(it->streamIndex);

        ports()->removePort(it->readPort->getName());
        delete it->readPort;

        ports()->removePort(it->write_port->getName());
        delete it->write_port;
    }

    for(uint i = 0; i < mVectors.size(); i++) {

        ports()->removePort(mVectors[i].rawOut->getName());
        delete mVectors[i].rawOut;

        if ( mVectors[i].debugOut ){
            ports()->removePort(mVectors[i].debugOut->getName());
            delete mVectors[i].debugOut;
        }

    }

    mDataInfos.clear();
    mVectors.clear();
}

bool BaseTask::isDataAvailable () const {

    if (mVectors.empty()) return false;

    Vectors::const_iterator it = mVectors.begin();

    for ( ; it != mVectors.end(); it++)
        if ( !it->isFilled() ) return false;

    return true;
}

void BaseTask::process(){

    //Default behavior: Simply forward the incoming data to output ports. If you want to
    //do data processing, overwrite this method in derived task context
    for ( uint i = 0; i < mVectors.size(); i++ ) {

        base::VectorXd vect;
        getVector(i, vect);
        setOutputVector(i, vect);
    }
}

void BaseTask::updateData() {
    
    DataInfos::iterator data_it = mDataInfos.begin();

    for ( ; data_it != mDataInfos.end(); data_it++ ) {
        while ( data_it->update(_create_places.get()) );
    }
    
    while ( _stream_aligner.step() );

    Vectors::iterator vector_it = mVectors.begin();
    for ( ; vector_it != mVectors.end(); vector_it++ )
        vector_it->writeRaw();

    if ( _debug_conversion.get() ) {
        vector_it = mVectors.begin();
        for ( ; vector_it != mVectors.end(); vector_it++ )
            vector_it->writeDebug();
    }

}

bool BaseTask::configureHook()
{
    if (! BaseTaskBase::configureHook())
        return false;

    std::vector<PortConfig> port_config = _port_config.get();
    for(uint i = 0; i < port_config.size(); i++)
    {
        if(!addPort(port_config[i]))
            return false;
    }
 
    Vectors::iterator it = mVectors.begin();
    for (int idx = 0; it != mVectors.end(); it++, idx++ ) {

        if (!addRawOutput(*it, idx)){
            log(Error) << "couldn't create raw output port for vector " << idx << endlog();
            return false;
        }
    }

   if ( _debug_conversion.get() ) { 
        it = mVectors.begin();
        for (int idx = 0; it != mVectors.end(); it++, idx++ ) {
        
            if (!it->debugOut && !addDebugOutput(*it, idx))
                log(Warning) << "couldn't create debug output port for vector " << idx << endlog();
        }
    }

    return true;
}

bool BaseTask::startHook() {

    base::Time t_now = base::Time::now();
    base::Time delta_time = t_now - _start_time.get();

    log(Info) << "started at time " << t_now.toString() << " with a delta= " <<
        delta_time.toSeconds() << endlog();
    
    DataInfos::iterator it = mDataInfos.begin();

    for (; it != mDataInfos.end(); it++) {

        it->start = _start_time.get();
        it->delta = delta_time;
    }

    return true;
}

void BaseTask::updateHook()
{
    updateData();
    if ( isDataAvailable() ){
        process();
        convertBackAndWrite();
    }

    BaseTaskBase::updateHook();
}

// void BaseTask::errorHook()
// {
//     BaseTaskBase::errorHook();
// }

void BaseTask::stopHook()
{
    BaseTaskBase::stopHook();
    _stream_aligner.clear();
}

void BaseTask::cleanupHook()
{
    BaseTaskBase::cleanupHook();
    clear(); 
}

