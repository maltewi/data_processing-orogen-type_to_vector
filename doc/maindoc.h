/** \mainpage Type To Vector
 *  
 * This package provides basic tasks to enable generic processing components.
 * Generic processing components have an algorithm implemented that works
 * with vectors or matricies rather than with specific data types. Therefore it
 * provides means to add ports of abritrary type and makes the data accesible 
 * inside the component as vector of doubles.
 *
 * \tableofcontents
 *
 * \section The Type To Vector Components
 *
 * \subsection intro Introduction
 *
 * This package contains two components as basis for generic data processing tasks:
 *  -# type_to_vector::BaseTask
 *  -# type_to_vector::BufferedDataTask
 *
 *  Both have an interface to create ports of abritary type and have the data
 *  available inside as vector of doubles. The \c BaseTask provides just the
 *  most recent data as \c base::VectorXd. \c BufferedDataTask buffers the vectors,
 *  and provides the vectors from the last \e n time steps. Those can be 
 *  obtained in from of a base::MatrixXd.
 *
 * \subsection component Creating your own component
 * 
 * For creating a component with a specific algorithm the new component must be 
 * a subclass of one of the above mentioned base classes. An orogen file may look
 * look like
 *
 * \code
 * name "my_generic_algorithm"
 *
 * version "0.1"
 *
 * using_task_library 'type_to_vector'
 *
 * task_context "Task" do
 *   subclasses 'type_to_vector::BaseTask'
 *
 *   ...
 * end
 * \endcode
 *
 * Of course the new package must depend on the type_to_vector package. Add
 *
 * \code
 * <depend package="data_processing/orogen/type_to_vector"/>
 * \endcode
 *
 * to the \e manifest.xml file.
 *
 * \subsection add Adding ports
 *
 * Port to be created are described with type_to_vector::PortConfig. It contains
 *  - \c portname
 *  - \c type
 *  - \c slice
 *  - \c vectorIdx
 *  - \c period
 *  - \c useTimeNow
 * 
 * To create a port give a type_to_vector::PortConfig structure to the addPort operation.
 * The port will be created immediatly. Note however that the debug port are created
 * during configuraiton.
 * For examples see the \e script folder of this package.
 *
 * \note 
 * It is important to give values for all fields of PortConfig, otherwise
 * uninitialized values could cause severe problems.
 *
 * \note
 * The type used for conversion might not be the type specified. In case
 * of opaques type, which typelib cannot directly understand and for which 
 * an intermediate type exists, the intermediate type is used. That is especially
 * important to consider when giving slices. Informations regarding the types can be
 * found on the <a href="http://rock-robotics.org/stable/types/index.html">rock-webpage
 * </a> and with \c rock-browse.
 *
 * If the property \c debug_conversion is true, a debug port (debug_<idx>) is created, 
 * that outputs the conversion result. <idx> is the index of the accociated vector. The 
 * port is of type  type_to_vector::ConvertedVector. For the 
 * type_to_vector::BufferedDataTask the property \c debug_buffer exists. If true, 
 * another port (debug_buffer_<idx>) is created that will show the buffer content: 
 * type_to_vector::BufferContent.
 *
 * \subsection time Timestamp
 *
 * For each port the time stamp is create from an appropiate field of the type, that
 * itself has the type base::Time. Normally the name is assumed to be "time". The
 * property \c time_fields allows to give the names of timestamp fields. The first field 
 * in a type with an matching name is taken as timestamp. 
 *
 * If there is no timestamp, the time stamp can be set in three different ways:
 *  -# if useTimeNow is set, base::Time::now() is used for time stamping,
 *  -# else if period is set the time stamp is start time + sample count * period or
 *  -# it is start time + time since calling the startHook.
 * The reference time is set with the proeprty \c start_time.
 *
 * \subsubsection slice Defining slices
 *
 * A slice defines which parts of a type shall be put into the vector.
 *  - Slices describe a place in the type. 
 *  - Slices are seperated by a space.
 *  - Levels are separated by a dot.
 *  - An inverse slice starting with '!' means that all described places should not be
 * in a vector.
 *  - Countable types like arrays or containers can be sliced with the valid indicies 
 * in brackets:
 *    - all indices: *
 *    - element at index 2: 2
 *    - range 1 - 10: [1-10]
 *    - range 2-30, every 2nd: [2-30:2] (every xth work only together with a range)
 *    - elements 0, 3 and 9: [0,3,9]
 *    - mix: [0,3-9,12,20-100:5]
 *
 * Assume:
 * \code
 * struct A {
 *  double a[3];
 *  int b;
 *  char c;
 * };
 * struct B {
 *  int idx;
 *  A data[5];
 * };
 * \endcode
 *
 *  - get everything of B: ""
 *  - get everything in B.data: "data"
 *  - get b of first and third B.data: "data.[0,2].b"
 *  - get idx and b of B.data: "idx data.*.b"
 *  - get field c of all B.data: "data.*.c"
 *  - get not field B.idx and not all B.data.b: "! idx data.*.b"
 *  - get nothing: "!"
 *
 * \subsection impl Implement the algorithm
 *
 * The setup and initialisation of the algorithm is as usual done in the 
 * \c conigurationHook and/or in the \c startHook. For the processing code
 * one must overwrite the method type_to_vector::BaseTask::process().
 * The task starts to call it when the vectors (for the type_to_vector::BaseTask) 
 * or the buffers (for the type_to_vector::BufferedDataTask) are filled once.
 * If those criterias to start the processing does not fit, the method
 * type_to_vector::BaseTask::isDataAvailable might be reimplemented.
 *
 * \subsection get Getting the data
 *
 * Per vector the infrastructure provides three different data:
 *  - the data,
 *  - the time stamps of the data and
 *  - place description where the data are from (which port, which part of the type).
 *  
 *  The latter one is mainly for debugging purposes and will only be created if the 
 *  property \c create_places is true. The methods to get the vectors are:
 *  - type_to_vector::BaseTask::getVector
 *  - type_to_vector::BaseTask::getExpandedTimeVector
 *  - type_to_vector::BaseTask::getPlaces
 *
 *  The methods take a reference to an object as argument to store the data in.
 *
 * \subsection buffer Use the buffer
 *
 * The type_to_vector::BufferedDataTask stores vectors in a buffer. That allows
 * to get a vector series of older data. The number of vectors stored is given with
 * the property \c buffer_size. The property \c buffer_time chooses whether to store
 * timestamp informations in a buffer or not.
 * The operation type_to_vector::BufferedDataTask::setBufferSizeFromTime allows
 * to give time parameters in [s] to determine and set the needed \c buffer_size.
 *
 * To get the date the following methods are available:
 * - type_to_vector::BufferedDataTask::getDataMatrix(int,int,int,base::MatrixXd&)
 * - type_to_vector::BufferedDataTask::getDataMatrix(int,double,double,base::MatrixXd&)
 * - type_to_vector::BufferedDataTask::getTimeMatrix(int,int,int,base::MatrixXd&)
 * - type_to_vector::BufferedDataTask::getTimeMatrix(int,double,double,base::MatrixXd&)
 * 
 * The \e int versions let one give the vectors to get in terms of indices. The
 * \e double versions let one give time values in [s]. The indices are resolved using the
 * period of the task (1.0 if not periodic).
 * \code
 *                Newest  Oldest
 * Vector Index   0 1 2 .. 7 .. n
 *             from ^   to ^
 * \endcode
 *
 * Negative numbers are allowed for \c from and \c to. Then they are meant from the end:
 * index = n - neg_index.
 *
 * \subsection rawport Use the raw input port
 *
 * For each port an additional input port is created that take base::VectorXd data. This
 * port can be used to directly feed vectorized data into the component. If there are
 * no data on the typed port the raw port is checked. Raw data have the same options for
 * time stamping as listed above for not time stamped data.
 * 
 */

