// 验证 null 后端播放推进：AL_BUFFERS_PROCESSED / AL_BYTE_OFFSET 是否随播放变化
#include <cstdio>
#include <hgl/audio/OpenAL.h>
#include <hgl/audio/AudioPlayer.h>
#include <hgl/time/Time.h>

using namespace hgl;
using namespace hgl::audio;

int main()
{
    fprintf(stderr,"InitOpenAL: %s\n", openal::InitOpenAL(nullptr,"null",false,false)?"OK":"FAIL");
    fflush(stderr);

    AudioPlayer *p=new AudioPlayer;
    fprintf(stderr,"Load: %s total=%f\n", p->Load(OS_TEXT("test_tone.wav"))?"OK":"FAIL", p->GetTotalTime());
    fflush(stderr);

    p->Play(false);
    fprintf(stderr,"Play called loop=%d\n", p->IsLoop()?1:0);
    fflush(stderr);

    for(int i=0;i<15;i++)
    {
        hgl::SleepSecond(0.2);

        openal::ALint processed=0, queued=0, offset=0, state=0;

        openal::alGetSourcei(p->GetIndex(),AL_BUFFERS_PROCESSED,&processed);
        openal::alGetSourcei(p->GetIndex(),AL_BUFFERS_QUEUED,&queued);
        openal::alGetSourcei(p->GetIndex(),AL_BYTE_OFFSET,&offset);
        openal::alGetSourcei(p->GetIndex(),AL_SOURCE_STATE,&state);

        fprintf(stderr,"  t=%4.1f processed=%d queued=%d offset=%d state=%d playstate=%d playtime=%f\n",
                i*0.2,processed,queued,offset,state,(int)p->GetPlayState(),p->GetPlayTime());
        fflush(stderr);
    }

    p->Stop();
    p->WaitExit(1.0);
    delete p;
    fprintf(stderr,"done\n");
    return 0;
}
