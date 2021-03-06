/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef TYPE_TO_VECTOR_BUFFEREDDATATASK_TASK_HPP
#define TYPE_TO_VECTOR_BUFFEREDDATATASK_TASK_HPP

#include "type_to_vector/BufferedDataTaskBase.hpp"

namespace type_to_vector {

    struct VectorBuffer;

    /*! \class BufferedDataTask 
     */
    class BufferedDataTask : public BufferedDataTaskBase
    {
	friend class BufferedDataTaskBase;

        typedef std::vector<VectorBuffer> Buffers;

        Buffers mBuffers;

    protected:
        
        /** Processing a sample is called by sampleCallback. */
        virtual void processSample(base::Time const& timestamp, SampleData const& sample);
        
        /** Adds data conversion informations. */ 
        virtual bool createDataInfo(const PortConfig& config);
        
        virtual void clear();


        /** Calculates the needed buffer size from times and durations. Set the
         * property buffer_size accordingly.
         */
        virtual boost::int32_t setBufferSizeFromTime(::base::Time const & delta_time, 
                ::base::Time const & window_start, ::base::Time const & window_length);

        /** To get the data matrix from the buffer. 
         *
         * \returns true if the buffer has a matrix.*/
        bool getDataMatrix(int vector_idx, int from, int to, base::MatrixXd& data_matrix);
        
        /** To get the data matrix from the buffer. 
         *
         * Uses getPeriod to resolve the indices. Period is assumed to be 1.0 in case of
         * non-periodic tasks.*/
        bool getDataMatrix(int vector_idx, double from_time, double to_time, 
                base::MatrixXd& data_matrix);

        /** To get the time matrix from the buffer. */
        bool getTimeMatrix(int vector_idx, int from, int to, base::MatrixXd& time_matrix);
        
        /** To get the time matrix from the buffer. Uses getPeriod to resolve the indices.*/
        bool getTimeMatrix(int vector_idx, double from_time, double to_time, 
                base::MatrixXd& time_matrix);

        /**
         * Computes the ratio of buffered samples that were copied
         * from the previous sample because of missing new samples
         * @param index index of the port
         * returns the ratio of buffered samples
         */
        double getBufferedSamplesRatio(unsigned int index);

        /** Is the Buffer filled.  */
        bool isBufferFilled(int vector_idx) const;

        /** For this task data are available when the buffers are filled once. */
        virtual bool isDataAvailable() const;

        /** .. and put them into the buffer. */
        virtual void updateData();

    public:
        /** TaskContext constructor for BufferedDataTask
         * \param name Name of the task. This name needs to be unique to make it identifiable via nameservices.
         */
        BufferedDataTask(std::string const& name = "type_to_vector::BufferedDataTask");

        /** TaskContext constructor for BufferedDataTask 
         * \param name Name of the task. This name needs to be unique to make it identifiable for nameservices. 
         * \param engine The RTT Execution engine to be used for this task, which serialises the execution of all commands, programs, state machines and incoming events for a task. 
         * 
         */
        BufferedDataTask(std::string const& name, RTT::ExecutionEngine* engine);

        /** Default deconstructor of BufferedDataTask
         */
	~BufferedDataTask();

        bool configureHook();

    };
}

#endif

