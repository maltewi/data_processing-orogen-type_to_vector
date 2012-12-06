// \file  DataStorage.cpp

#include <base/eigen.h>

#include <rtt/base/PortInterface.hpp>
#include <rtt/base/InputPortInterface.hpp>
#include <rtt/OutputPort.hpp>
#include <rtt/DataFlowInterface.hpp>

#include <aggregator/StreamAligner.hpp>

#include <general_processing/VectorBuilder.hpp>

#include "VectorDataStorage.hpp"

using namespace general_processing;
using RTT::log;
using RTT::endlog;
using RTT::Debug;
    
bool DataInfo::update(bool create_places) {
       
    if( readPort->read(sample, false) == RTT::NewData) {

        typelibMarshaller->refreshTypelibSample(handle);

        uint8_t* data_ptr = typelibMarshaller->getTypelibSample(handle);

        conversions.update(data_ptr, create_places);

        newSample.mData = conversions.getData(DATACONVERSION);

        base::Time t;

        if ( hasTime ) {
            newSample.mTime = conversions.getData(TIMECONVERSION).front();

            t = *(reinterpret_cast<base::Time*>( data_ptr + 
                        conversions.getToc(TIMECONVERSION).front().position) );
        } else {
            t = base::Time::now();
            newSample.mTime = t.toSeconds();
        } 
        
        if ( create_places )
            newSample.mPlaces = conversions.getPlaces(DATACONVERSION);
        else
            newSample.mPlaces.clear();

        pStreamAligner->push(streamIndex, t, newSample);

        return true;
    }  
    return false;
}
    

int DataVector::addVectorPart () {

    push_back(SampleData());

    return size()-1;
}

void DataVector::getVector (base::VectorXd& vector) const {

    int size = vectorSize();
    
    vector.resize(size);
 
    int pos = 0;
    const_iterator it = begin();
    for (; it != end(); it++) {
        int n = it->mData.size();
        if (n == 0) continue;
        vector.segment(pos, n) = Eigen::Map<const base::VectorXd>(&(it->mData[0]), n);
        pos += n;
    }
}

void DataVector::getTimeVector (base::VectorXd& time_vector) const {

    time_vector.resize(size());
    
    const_iterator it = begin();

    int i = 0;
    for (; it != end(); it++)
        time_vector[i++] = it->mTime;
}

void DataVector::getExpandedTimeVector (base::VectorXd& time_vector) const {
    
    int size = vectorSize();

    time_vector.resize(size);
    
    int pos = 0;
    const_iterator it = begin();
    for (; it != end(); it++) {
        int n = it->mData.size();
        if (n == 0) continue;
        time_vector.segment(pos, n) = base::VectorXd::Constant(n,it->mTime);
        pos += n;
    }
}

void DataVector::getPlacesVector (StringVector& places_vector) const {

    const_iterator it = begin();
    int size = 0;
    
    for ( ; it != end(); it++ )
        size += it->mPlaces.size();

    places_vector.clear();
    places_vector.reserve(size);

    for ( it = begin(); it != end(); it++ )
       places_vector.insert(places_vector.end(),it->mPlaces.begin(), it->mPlaces.end());
}

ConvertedVector& DataVector::getConvertedVector (ConvertedVector& data ) const {

    getVector(data.data);
    getExpandedTimeVector(data.time);
    getPlacesVector(data.places);
    return data;
}

void DataVector::writeDebug() {

    if ( !debugOut && wroteDebug ) return;
    
    static ConvertedVector data;
    static_cast<RTT::OutputPort<ConvertedVector>*>(debugOut)->
        write(getConvertedVector(data));
    wroteDebug = true;
}

bool DataVector::isFilled() const {

    for ( const_iterator it = begin(); it != end(); it++)
        if ( it->mData.empty() ) return false;

    return true;
}

int DataVector::vectorSize() const {

    const_iterator it = begin();
    int size = 0;

    for ( ; it != end(); it++ )
        size += it->mData.size();

    return size;
}

bool VectorBuffer::create (const DataVector& dv, int vector_count, bool buffer_time) {

    if ( !dv.isFilled() ) return false;

    if ( mDataBuffer.get()) 
        if ( mDataBuffer->getVectorSize() == dv.vectorSize() && 
                mDataBuffer->getVectorCount() == vector_count ) 
            return true; 
        else 
            return false;

    mDataBuffer.reset( new MatrixBuffer(dv.vectorSize(), vector_count) );
    
    if ( buffer_time ) 
        mTimeBuffer.reset( new MatrixBuffer(dv.vectorSize(), vector_count) );

    return true;
}

void VectorBuffer::push (const DataVector& dv) {

    dv.getVector(mStoreVector);

    mDataBuffer->push(mStoreVector);

    if ( mTimeBuffer.get() ) {
        dv.getExpandedTimeVector(mStoreVector);
        mTimeBuffer->push(mStoreVector);
    }
}

bool VectorBuffer::getDataMatrix (int from, int to, base::MatrixXd& matrix) {

    if ( !mDataBuffer.get() ) return false;

    Eigen::MatrixXd mat;

    mDataBuffer->getMatrix(from,to,mat);
    matrix = mat;
    return true;
}

bool VectorBuffer::getTimeMatrix (int from, int to, base::MatrixXd& matrix) {

    if ( !mTimeBuffer.get() ) return false;

    mTimeBuffer->getMatrix(from,to,matrix);
    return true;
}

bool VectorBuffer::getDataMatrix (double from_time, double to_time, double delta_time,
        base::MatrixXd& matrix) {

    int from = from_time / delta_time;
    int to = to_time / delta_time;

    return getDataMatrix(from,to,matrix);
}

bool VectorBuffer::getTimeMatrix (double from_time, double to_time, double delta_time,
        base::MatrixXd& matrix) {

    int from = from_time / delta_time;
    int to = to_time / delta_time;

    return getTimeMatrix(from,to,matrix);
}
