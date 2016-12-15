
#ifndef zaudio_hpp
#define zaudio_hpp

/*
This file is part of zaudio.

    zaudio is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    zaudio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with zaudio.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstddef>
#include <cstdint>
#include <utility>
#include <functional>
#include <vector>
#include <chrono>
#include <type_traits>
#include <string>
#include <sstream>
#include <memory>
#include <ostream>
#include <future>

/*
TODO: Documentation on everything, testing on lots of things
*/
/*!
 *\namespace zaudio
 *\brief
 */
namespace zaudio
{
    /*!
     *\enum sample_format
     *\brief
     */
    enum class sample_format
    {
        f32,
        f64,
        u8,
        i8,
        i16,
        i24,
        i32,
        i64,
        err
    };

    std::ostream& operator<<(std::ostream& os, sample_format format);

    /*!
     *\namespace
     *\brief
     */
    namespace detail
    {
        #pragma pack(push)
        //TODO: make me better


        /*!
         *\struct
         *\brief
         */
        struct s24
        {
            int value:3;
            inline operator int()
            {
                return value;
            }
            inline s24& operator=(int v)
            {
                value = v;
                return *this;
            }
        };
        #pragma pack(pop)


        /*!
         *\struct
         *\brief
         */
        template<sample_format> struct format_id_to_type        { using type = void;         };
        template<> struct format_id_to_type<sample_format::f32> { using type = float;        };
        template<> struct format_id_to_type<sample_format::f64> { using type = double;       };
        template<> struct format_id_to_type<sample_format::i8>  { using type = std::int8_t;  };
        template<> struct format_id_to_type<sample_format::u8>  { using type = std::uint8_t;  };
        template<> struct format_id_to_type<sample_format::i16> { using type = std::int16_t; };
        template<> struct format_id_to_type<sample_format::i24> { using type = s24;          };
        template<> struct format_id_to_type<sample_format::i32> { using type = std::int32_t; };
        template<> struct format_id_to_type<sample_format::i64> { using type = std::int64_t; };

        /*!
         *\struct
         *\brief
         */
        template<typename T> struct type_to_format_id     { constexpr static sample_format value = sample_format::err; };
        template<> struct type_to_format_id<float>        { constexpr static sample_format value = sample_format::f32; };
        template<> struct type_to_format_id<double>       { constexpr static sample_format value = sample_format::f64; };
        template<> struct type_to_format_id<std::int8_t>  { constexpr static sample_format value = sample_format::i8;  };
        template<> struct type_to_format_id<std::uint8_t> { constexpr static sample_format value = sample_format::u8;  };
        template<> struct type_to_format_id<std::int16_t> { constexpr static sample_format value = sample_format::i16; };
        template<> struct type_to_format_id<s24>          { constexpr static sample_format value = sample_format::i24; };
        template<> struct type_to_format_id<std::int32_t> { constexpr static sample_format value = sample_format::i32; };
        template<> struct type_to_format_id<std::int64_t> { constexpr static sample_format value = sample_format::i64; };

        /*!
         *\fn
         *\brief
         */
        template<typename T>
        constexpr bool is_sample_type() noexcept
        {
            return type_to_format_id<T>::value != sample_format::err;
        }

        /*!
         *\struct
         *\brief
         */
        template<typename T,sample_format f = type_to_format_id<T>::value>
        struct fail_if_type_is_not_sample : type_to_format_id<T>
        {};

        template<typename T>
        struct fail_if_type_is_not_sample<T,sample_format::err> : type_to_format_id<T>
        {
            static_assert(is_sample_type<T>(),"Invalid sample type provided!");
        };

    }


    /*!
     *\typedef
     *\brief
     */
    template<sample_format F>
    using sample = typename detail::format_id_to_type<F>::type;

    /*!
     *\fn
     *\brief
     */
    template<sample_format format>
    constexpr static std::size_t sample_size() noexcept
    {
        return sizeof(std::declval<typename detail::format_id_to_type<format>::type>());
    }

    /*!
     *\fn
     *\brief
     */
    constexpr static std::size_t sample_size(sample_format format) noexcept
    {
        //REPLACE WITH SWITCH IN C++14
        return format == sample_format::f32 ? sample_size<sample_format::f32>() :
              (format == sample_format::f64 ? sample_size<sample_format::f64>() :
              (format == sample_format::i8  ? sample_size<sample_format::i8> () :
              (format == sample_format::u8  ? sample_size<sample_format::u8> () :
              (format == sample_format::i16 ? sample_size<sample_format::i16>() :
              (format == sample_format::i24 ? sample_size<sample_format::i24>() :
              (format == sample_format::i32 ? sample_size<sample_format::i32>() :
              (format == sample_format::i64 ? sample_size<sample_format::i64>() :
              /*Error Case*/0)))))));
    }


    /*!
    *\typedef audio_clock
    *\brief a clock guarenteed to be monotonic.
    *\note std::chrono::high_resolution_clock will be used only when std::chrono::high_resolution_clock::is_steady == true
    */
    using audio_clock = typename std::conditional<std::chrono::high_resolution_clock::is_steady,
                                                  std::chrono::high_resolution_clock,
                                                  std::chrono::steady_clock>::type;
    using duration = std::chrono::duration<double>;
    using time_point = audio_clock::time_point;


    /*!
     *\enum stream_status
     *\brief a list of status codes to be used for realtime error signaling and messages
     */
    /*------------------------------------------------------------------------*/
    enum class stream_status
    {
        //valid states
        running,
        stopped,
        paused,
        //error states
        no_error,
        fatal_error,
        xrun,
        system_error,
        user_error
    };
    std::string stream_status_string(stream_status status);
    std::ostream& operator<<(std::ostream& os,stream_status status);
    /*------------------------------------------------------------------------*/
    /*!
     *\typedef
     *\brief
     */
    using stream_error_message = const char*;

    /*!
     *\namespace
     *\brief
     */
    namespace detail
    {
        //a workaround for c++11 std::pair not having a constexpr contructor
        struct stream_error_type
        {
            stream_error_type():first(stream_status::no_error),second(""){}
            constexpr stream_error_type(stream_status f,stream_error_message s):first(f),second(s){}
            stream_status first;
            stream_error_message second;
            friend bool operator==(const stream_error_type& lhs,const stream_error_type& rhs)
            {
                return lhs.first == lhs.first && rhs.second == rhs.second;
            }
            friend bool operator!=(const stream_error_type& lhs,const stream_error_type& rhs)
            {
                return lhs.first != lhs.first || rhs.second != rhs.second;
            }
        };
    }


    /*!
     *\typedef
     *\brief
     */
    //use std pair if c++14 else use stream_error_type
    using stream_error = std::conditional<__cplusplus == 201402L,std::pair<stream_status, stream_error_message>,detail::stream_error_type>::type;

    std::ostream& operator<<(std::ostream& os, const stream_error& err);

    /*!
     *\fn
     *\brief
     */
    constexpr stream_error make_stream_error(stream_status status, stream_error_message code) noexcept
    {
        return stream_error(status,code);
    }

    /*!
     *\fn
     *\brief
     */
    constexpr stream_error make_stream_error(stream_status status) noexcept
    {
        return make_stream_error(status,"");
    }


    /*!
     *\class
     *\brief
     */
    class stream_exception : public std::runtime_error
    {
    public:
        explicit stream_exception(const char* what_arg);
        explicit stream_exception(const std::string& what_arg);
        explicit stream_exception(const stream_error& err);
        explicit stream_exception(const stream_status& status);
        explicit stream_exception(const stream_status& status,const stream_error_message& code);
    private:
        const std::string _make_error_string(const stream_error& err);
    };

    //user defined error callback type
    //can throw
    //is called by host when an error code is reported
    /*!
     *\typedef
     *\brief
     */
    using stream_error_callback = std::function<void(const stream_error&) noexcept>;



    /*!
     *\fn
     *\brief
     */
    inline stream_error_callback default_stream_error_callback() noexcept
    {
        return [](const stream_error& err)
        {
            std::async(std::launch::async, [](const stream_error& err)
            {
                throw stream_exception(err);
            }, err);

        };
    }


    //global stream_error codes for general use
    constexpr static stream_error running = make_stream_error(stream_status::running,"running");
    constexpr static stream_error stopped = make_stream_error(stream_status::stopped,"stopped");
    constexpr static stream_error paused  = make_stream_error(stream_status::paused,"paused");
    constexpr static stream_error no_error = make_stream_error(stream_status::no_error,"no error");





    /*!
     *\class stream_params
     *\brief
     */
    /*------------------------------------------------------------------------*/

    template<typename sample_t>
    class stream_params : public detail::fail_if_type_is_not_sample<sample_t>
    {
    public:
        constexpr stream_params() noexcept: _input_frame_width(2),_output_frame_width(2),_frame_count(512),_sample_rate(44100),_input_device_id(-1),_output_device_id(-1)
        {}
        constexpr stream_params(double sr, std::size_t fc, std::size_t fw) noexcept: _input_frame_width(fw),_output_frame_width(fw),_frame_count(fc),_sample_rate(sr),_input_device_id(-1),_output_device_id(-1)
        {}
        constexpr stream_params(double sr, std::size_t fc, std::size_t ifw, std::size_t ofw) noexcept : _input_frame_width(ifw),_output_frame_width(ofw),_frame_count(fc),_sample_rate(sr),_input_device_id(-1),_output_device_id(-1)
        {}
        constexpr stream_params(double sr, std::size_t fc, std::size_t ifw, std::size_t ofw,long idid,long odid) noexcept : _input_frame_width(ifw),_output_frame_width(ofw),_frame_count(fc),_sample_rate(sr),_input_device_id(idid),_output_device_id(odid)
        {}

        constexpr const std::size_t& frame_count() const noexcept
        {
            return _frame_count;
        }
        constexpr const std::size_t& input_frame_width() const noexcept
        {
            return _input_frame_width;
        }
        constexpr const std::size_t& output_frame_width() const noexcept
        {
            return _output_frame_width;
        }

        constexpr const std::size_t input_sample_count() const noexcept
        {
            return _input_frame_width * _frame_count;
        } // frame_width * frame_count
        constexpr const std::size_t output_sample_count() const noexcept
        {
            return _output_frame_width * _frame_count;
        } // frame_width * frame_count

        constexpr const double& sample_rate() const noexcept
        {
            return _sample_rate;
        }
        constexpr const double input_byte_rate() const noexcept
        {
            return sample_rate() * input_sample_count() * sizeof(sample_t);
        }
        constexpr const double input_bit_rate() const noexcept
        {
            return input_byte_rate() * 8;
        }
        constexpr const double output_byte_rate() const noexcept
        {
            return sample_rate() * output_sample_count() * sizeof(sample_t);
        }
        constexpr const double output_bit_rate() const noexcept
        {
            return output_byte_rate() * 8;
        }
        constexpr const long& input_device_id() const noexcept
        {
            return _input_device_id;
        }
        constexpr const long& output_device_id() const noexcept
        {
            return _output_device_id;
        }
        friend std::ostream& operator<<(std::ostream& os, stream_params<sample_t>& params)
        {
            os<<"Input Frame Width: "<<params.input_frame_width()<<std::endl;
            os<<"Output Frame Width: "<<params.output_frame_width()<<std::endl;
            os<<"Frame Count: "<<params.frame_count()<<std::endl;
            os<<"Sample Rate: "<<params.sample_rate()<<std::endl;
            os<<"Input Device ID: "<<params.input_device_id()<<std::endl;
            os<<"Ouput Device ID: "<<params.output_device_id()<<std::endl;
            return os;
        }

    private:
        std::size_t _input_frame_width;
        std::size_t _output_frame_width;
        std::size_t _frame_count;
        double _sample_rate;
        long _input_device_id;
        long _output_device_id;
    };
    /*------------------------------------------------------------------------*/



    /*!
     *\fn make_stream_params
     *\brief
     */
    /*------------------------------------------------------------------------*/
    template<typename sample_t, typename... args_t>
    constexpr stream_params<sample_t> make_stream_params(args_t&&...args) noexcept
    {
        return stream_params<sample_t>(std::forward<args_t&&>(args)...);
    }
    /*------------------------------------------------------------------------*/

    template<typename sample_t>
    using stream_callback  = std::function<stream_error(const sample_t*, sample_t*,duration, stream_params<sample_t>&) noexcept>;



    struct device_info
    {
        device_info() noexcept;
        device_info(const char* nm,
                    std::size_t idx,
                    std::size_t mic,
                    std::size_t moc,
                    double srate,
                    duration dlil,
                    duration dhil,
                    duration dlol,
                    duration dhol) noexcept;


        const char* name;
        std::size_t device_index;
        std::size_t max_input_count;
        std::size_t max_output_count;
        double default_sample_rate;
        duration default_low_input_latency;
        duration default_high_input_latency;
        duration default_low_ouput_latency;
        duration default_high_output_latency;

        friend std::ostream& operator<<(std::ostream& os, const device_info& info);
    };

    template<typename... args_t>
    device_info make_device_info(args_t&&... args) noexcept
    {
        return device_info(std::forward<args_t&&>(args)...);
    }


    /*!
     *\class stream_api
     *\brief
     */
    /*------------------------------------------------------------------------*/
    template<typename sample_t>
    class stream_api : public detail::fail_if_type_is_not_sample<sample_t>
    {
    public:
        using callback = stream_callback<sample_t>;
        using error_callback = stream_error_callback;

        stream_api() noexcept: _callback(nullptr),_error_callback(nullptr){}

        //id will be assigned based on std::hash<std::string> of name()
        //aka: unique name = unique id
        std::size_t id() const noexcept
        {
            return std::hash<std::string>{}(name());

        }
        virtual std::string name() const noexcept
        {
            return "dummy";
        }
        virtual std::string info() const noexcept
        {
            return "dummy";
        }
        virtual stream_error start() noexcept
        {
            return make_stream_error(stream_status::system_error,"Attempted use of dummy API");
        }
        virtual stream_error pause() noexcept
        {
            return make_stream_error(stream_status::system_error,"Attempted use of dummy API");
        }
        virtual stream_error stop() noexcept
        {
            return make_stream_error(stream_status::system_error,"Attempted use of dummy API");
        }
        virtual stream_error playback_state() noexcept
        {
            return make_stream_error(stream_status::system_error,"Attempted use of dummy API");
        }
        virtual std::string get_error_string(const stream_error& err) noexcept
        {
            return {};
        }
        virtual stream_error open_stream(const stream_params<sample_t>&) noexcept
        {
            return make_stream_error(stream_status::system_error,"Attempted use of dummy API");
        }
        virtual stream_error close_stream() noexcept
        {
            return make_stream_error(stream_status::system_error,"Attempted use of dummy API");
        }
        virtual long get_device_count() noexcept
        {
            return 0;
        }
        virtual device_info get_device_info(long id) noexcept
        {
            return {};
        }
        virtual stream_error is_configuration_supported(const stream_params<sample_t>& params) noexcept
        {
            return make_stream_error(stream_status::system_error,"Attempted use of dummy API");
        }
        virtual long default_input_device_id() const noexcept
        {
            return 0;
        }
        virtual long default_output_device_id() const noexcept
        {
            return 0;
        }



        void set_callback(callback& cb) noexcept
        {
            _callback = &cb;
        }
        void set_error_callback(error_callback&cb) noexcept
        {
            _error_callback = &cb;
        }


        /*
        NOTE: INTERFACE TBD
        */
    protected:
        callback* _callback;
        error_callback* _error_callback;
    };

    /*!
    *\note At least one stream api must be present on every platform.
           The provided api is permitted to be a non functional dummy api for platforms where a proper api is impossible
           Preferabbly on any platform that has a filesystem, a file codec api will be provided
           Preferabbly on any platform that has an audio system an hardware api will be provided

           IDEA:
                Network serialization api?
                    an api that is designed to work with the std::networking proposal for sending audio over network

           All of the apis will share a uniform interface
           Ideally multiple apis can be used at once
                TODO: come up with endpoint class to make multi api io possible
    */
    /*------------------------------------------------------------------------*/

    /*!
     *\fn default_api
     *\brief
     */
    /*------------------------------------------------------------------------*/
    template<typename sample_t>
    std::unique_ptr<stream_api<sample_t>> default_api() noexcept;
    /*------------------------------------------------------------------------*/

    /*!
     *\fn
     *\brief
     */
    template<typename sample_t,template<typename> class api>
    std::unique_ptr<stream_api<sample_t>> make_stream_api() noexcept
    {
        return std::unique_ptr<stream_api<sample_t>>{new api<sample_t>{}};
    }




    /*!
     *\class stream_context
     *\brief
     * NOTE: DOES NOT ACCOUNT FOR MULTIPLE STREAMS YET
     * TODO: Account for multiple streams / APIs
     */

    /*------------------------------------------------------------------------*/
    template<typename sample_t>
    class stream_context : public detail::fail_if_type_is_not_sample<sample_t>
    {
    public:

        using api_type = stream_api<sample_t>;
        using stream_params_type = stream_params<sample_t>;
        using callback = stream_callback<sample_t>;

        explicit stream_context(std::unique_ptr<api_type> api = default_api<sample_t>()) noexcept:_api(std::move(api))
        {}

        api_type* api() const noexcept
        {
            return _api.get();
        }//get the api


        long get_device_count() noexcept
        {
            return _api.get()->get_device_count();
        }
        device_info get_device_info(long id) noexcept
        {
            return _api.get()->get_device_info(id);
        }
        std::vector<device_info> get_device_info_list() noexcept
        {
            std::vector<device_info> out;
            for(long i=0;i<get_device_count();++i)
            {
                out.push_back(get_device_info(i));
            }
            return out;
        }
        stream_error is_configuration_supported(const stream_params_type& params) noexcept
        {
            return _api.get()->is_configuration_supported(params);
        }
        long default_input_device_id() const noexcept
        {
            return _api.get()->default_input_device_id();
        }
        long default_output_device_id() const noexcept
        {
            return _api.get()->default_output_device_id();
        }
    private:
        std::unique_ptr<api_type> _api;
    };
    /*------------------------------------------------------------------------*/



    /*!
     *\fn make_stream_context
     *\brief
     */
    /*------------------------------------------------------------------------*/
    template<typename sample_t,typename... args_t>
    stream_context<sample_t> make_stream_context(args_t&&... args) noexcept
    {
        return stream_context<sample_t>{std::forward<args_t&&>(args)...};
    }

    /*------------------------------------------------------------------------*/


    /*!
     *\fn default_stream_context
     *\brief
     */
    /*------------------------------------------------------------------------*/

    template<typename sample_t>
    stream_context<sample_t> default_stream_context() noexcept;

    /*------------------------------------------------------------------------*/

    template<typename sample_t>
    struct audio_process;


    /*!
     *\class audio_stream
     *\brief
     */
    /*------------------------------------------------------------------------*/
    template<typename sample_t>
    class audio_stream : protected stream_params<sample_t>
    {
    public:
        using callback = stream_callback<sample_t>;
        using context_type = stream_context<sample_t>;
        using stream_params_type = stream_params<sample_t>;
        audio_stream() : stream_params_type(),_context(default_stream_context<sample_t>()),
                                              _error_callback(default_stream_error_callback())
        {
            init();
        }
        audio_stream(const stream_params_type& params,
                     const callback& cb,
                     const stream_error_callback& error_callback = default_stream_error_callback()) :stream_params_type(params),
                                                                                                     _context(default_stream_context<sample_t>()),
                                                                                                     _callback(cb),
                                                                                                     _error_callback(error_callback)
        {
            init();
        }
        audio_stream(const stream_params_type& params,
                     context_type& ctx,
                      const callback& cb,
                      const stream_error_callback& error_callback = default_stream_error_callback()):stream_params_type(params),
                                                                                                     _callback(cb),
                                                                                                     _error_callback(error_callback),
                                                                                                     _context(ctx)
        {
            init();
        }
        audio_stream(const stream_params_type& params,
                     audio_process<sample_t>& proc):stream_params_type(params),
                                                    _callback(proc.get_callback()),
                                                    _error_callback(proc.get_error_callback()),
                                                    _context(default_stream_context<sample_t>())
        {
            init();
        }
        audio_stream(const stream_params_type& params,
                     context_type& ctx, audio_process<sample_t>& proc):stream_params_type(params),
                                                                       _callback(proc.get_callback()),
                                                                       _error_callback(proc.get_error_callback()),
                                                                       _context(ctx)
        {
            init();
        }
        ~audio_stream()
        {
            destroy();
        }
        stream_error start() noexcept
        {
            return _context.get().api()->start();
        }
        stream_error pasue() noexcept
        {
            return _context.get().api()->pause();
        }
        stream_error stop() noexcept
        {
            return _context.get().api()->stop();
        }
        stream_error playback_state() noexcept
        {
            return _context.get().api()->playback_state();
        }
        callback exchange_callback(callback&& cb) noexcept
        {
            std::swap(_callback,cb);
            return cb;
        }
        stream_error_callback exchange_error_callback(stream_error_callback&& cb) noexcept
        {
            std::swap(_error_callback,cb);
            return cb;
        }

        const stream_params_type& params() noexcept
        {
            return *(stream_params_type*)this;

        }
        void params(const stream_params_type& p) noexcept
        {
            *(stream_params_type*)this = p;
        }

    private:
        void init()
        {
            _context.get().api()->set_callback(_callback);
            _context.get().api()->set_error_callback(_error_callback);
            auto&& is_compat = _context.get().is_configuration_supported(params());
            if(is_compat != no_error)
            {
                throw stream_exception(is_compat);
            }
            _context.get().api()->open_stream(params());
        }
        void destroy() noexcept
        {
            _context.get().api()->close_stream();

        }
        callback _callback;
        stream_error_callback _error_callback;
        std::reference_wrapper<context_type> _context;
    };
    /*------------------------------------------------------------------------*/



    /*!
     *\fn make_audio_stream
     *\brief
     */
    /*------------------------------------------------------------------------*/
    template<typename sample_t,typename... args_t>
    audio_stream<sample_t> make_audio_stream(args_t&&... args)
    {
        return audio_stream<sample_t>(args...);
    }
    /*------------------------------------------------------------------------*/


    /*!
     *\class audio_process
     *\brief
     */
    template<typename sample_t>
    struct audio_process : public detail::fail_if_type_is_not_sample<sample_t>
    {

        using callback = stream_callback<sample_t>;
        using error_callback = stream_error_callback;

        virtual stream_error on_process(const sample_t*,sample_t*,duration,stream_params<sample_t>&) noexcept
        {
            return no_error;
        }
        virtual void on_error(const stream_error& err) noexcept
        {
            throw stream_exception(err);
        }
        callback get_callback() noexcept
        {
            using std::placeholders::_1;
            using std::placeholders::_2;
            using std::placeholders::_3;
            using std::placeholders::_4;
            return callback(std::bind(&audio_process<sample_t>::on_process,this,_1,_2,_3,_4));
        }
        error_callback get_error_callback() noexcept
        {
            using std::placeholders::_1;
            return std::bind(&audio_process<sample_t>::on_error,this,_1);
        }
    };
}


#include "pa_stream_api.hpp"

namespace zaudio
{


    /*!
     *\fn
     *\brief
     */
    template<typename sample_t>
    std::unique_ptr<stream_api<sample_t>> default_api() noexcept
    {
        return std::unique_ptr<stream_api<sample_t>>{new pa_stream_api<sample_t>{}};
    }

    /*!
     *\fn
     *\brief
     */
    template<typename sample_t>
    stream_context<sample_t> default_stream_context() noexcept
    {
        return stream_context<sample_t>{};
    }
}

#endif
