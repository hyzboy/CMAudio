#ifndef HGL_AUDIO_SCENE_INCLUDE
#define HGL_AUDIO_SCENE_INCLUDE

#include<hgl/math/Math.h>
#include<hgl/type/Pool.h>
#include<hgl/type/Map.h>
#include<hgl/audio/ConeAngle.h>
#include<hgl/thread/ThreadMutex.h>
namespace hgl
{
    class AudioBuffer;
    class AudioSource;
    class AudioListener;

    /**
     * 邏輯發聲源
     */
    struct AudioSourceItem
    {
        friend const double GetGain(AudioListener *,AudioSourceItem *);
        friend class AudioScene;

    private:

        AudioBuffer *buffer;

    public:

        bool loop;                                          ///<是否循环播放
//         float min_gain,max_gain;
        float gain;                                         ///<音量增益

        uint distance_model;                                ///<音量衰减模型
        float rolloff_factor;                               ///<环境衰减比率,默认为1
        float doppler_factor;                               ///<多普勒效果强度,默认为0
        float air_absorption_factor;                        ///<空气吸收因子,默认为0

        float ref_distance;                                 ///<衰减距离
        float max_distance;                                 ///<最大距离
        ConeAngle cone_angle;
        Vector3f velocity;
        Vector3f direction;

    private:

        double start_play_time;                             ///<开播时间
        bool is_play;                                       ///<是否需要播放
        bool position_initialized;                          ///<位置是否已初始化

        Vector3f last_pos;
        double last_time;

        Vector3f cur_pos;
        double cur_time;

        double move_speed;

        double last_gain;                                   ///<最近一次的音量

        AudioSource *source;

    public:

        /**
         * 构造函数：初始化所有成员变量
         */
        AudioSourceItem()
            : buffer(nullptr)
            , loop(false)
            , gain(1.0f)
            , distance_model(0)
            , rolloff_factor(1.0f)
            , doppler_factor(0.0f)
            , air_absorption_factor(0.0f)
            , ref_distance(1.0f)
            , max_distance(10000.0f)
            , start_play_time(0)
            , is_play(false)
            , position_initialized(false)
            , last_time(0)
            , cur_time(0)
            , move_speed(0)
            , last_gain(0)
            , source(nullptr)
        {
            velocity = Vector3f(0, 0, 0);
            direction = Vector3f(0, 0, 0);
            last_pos = Vector3f(0, 0, 0);
            cur_pos = Vector3f(0, 0, 0);
        }

        /**
         * 请求播放这个音源
         * @param play_time 开播时间(默认为0，表示自能听到后再响)
         */
        void Play(const double play_time=0)
        {
            start_play_time=play_time;
            is_play=true;
            // 不重置 last_time，避免破坏 MoveTo() 的逻辑
        }

        /**
         * 请求立即停播这个音源
         */
        void Stop()
        {
            is_play=false;
        }

        /**
         * 移动音源到指定位置
         * @param pos 坐标
         * @param ct 当前时间
         */
        void MoveTo(const Vector3f &pos,const double &ct)
        {
            if(!position_initialized)
            {
                last_pos=cur_pos=pos;
                last_time=cur_time=ct;

                move_speed=0;
                position_initialized=true;
            }
            else
            {
                last_pos=cur_pos;
                last_time=cur_time;

                cur_pos=pos;
                cur_time=ct;
            }
        }
    };//struct AudioSourceItem

    const double GetGain(AudioListener *,AudioSourceItem *);                                        ///<取得相對音量

    /**
     * 音频场景管理
     */
    class AudioScene                                                                                ///<音频场景管理
    {
    protected:

        double cur_time;                                                                            ///<当前时间

        float ref_distance;                                                                         ///<默认1000
        float max_distance;                                                                         ///<默认100000

        AudioListener *listener;                                                                    ///<收聽者

        ObjectPool<AudioSource> source_pool;                                                        ///<音源数据池
        SortedSets<AudioSourceItem *> source_list;                                                  ///<音源列表
        
        ThreadMutex scene_mutex;                                                                    ///<线程互斥锁

    protected:

        bool ToMute(AudioSourceItem *);                                                             ///<转静默处理
        bool ToHear(AudioSourceItem *);                                                             ///<转发声处理

        bool UpdateSource(AudioSourceItem *);                                                       ///<刷新音源处理

    public:     //事件

        virtual float   OnCheckGain(AudioSourceItem *asi)                                           ///<檢測音量事件
        {
            return asi?GetGain(listener,asi)*asi->gain:0;
        }

        virtual void    OnToMute(AudioSourceItem *){/*無任何處理，請自行重載處理*/}                 ///<从有声变成聽不到聲音
        virtual void    OnToHear(AudioSourceItem *){/*無任何處理，請自行重載處理*/}                 ///<从听不到声变成能听到声音

        virtual void    OnContinuedMute(AudioSourceItem *){/*無任何處理，請自行重載處理*/}          ///<持续聽不到聲音
        virtual void    OnContinuedHear(AudioSourceItem *){/*無任何處理，請自行重載處理*/}          ///<持续可以聽到聲音

        virtual bool    OnStopped(AudioSourceItem *){return true;}                                  ///<单个音源播放结束事件,返回TRUE表示可以释放这个音源，返回FALSE依然占用这个音源

    public:

        AudioScene(int max_source,AudioListener *al);                                               ///<构造函数(指定最大音源数)
        virtual ~AudioScene()=default;                                                              ///<析构函数

                void                SetListener(AudioListener *al)                                 ///<設置收聽者
                {
                    if(al)  // 只有非空时才设置
                        listener=al;
                }

                /**
                 * 设定距离场数据，无预设单位，按自行使用单位设定即可。但后期坐标等数据单位需与次相同
                 * @param rd 衰减距离
                 * @param md 最大距离
                 */
                void                SetDistance(const float &rd,const float &md)                    ///<设定参考距离
                {
                    ref_distance=rd;
                    max_distance=md;
                }

        virtual AudioSourceItem *   Create(AudioBuffer *,const Vector3f &pos,const float &gain=1);  ///<创建一個音源
        virtual void                Delete(AudioSourceItem *);                                      ///<删除一个音源

        virtual void                Clear();                                                        ///<清除所有音源

        virtual int                 Update(const double &ct=0);                                     ///<刷新,返回仍在發聲的音源數量
    };//class AudioScene
}//namespace hgl
#endif//HGL_AUDIO_SCENE_INCLUDE
