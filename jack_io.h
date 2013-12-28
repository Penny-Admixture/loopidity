
#ifndef _JACK_IO_H_
#define _JACK_IO_H_


#include <jack/jack.h>
typedef jack_default_audio_sample_t Sample ;
#include "loopidity.h"
class Loop ;
class Scene ;


using namespace std ;


class JackIO
{
  private:

    /* JackIO class side private constants */

    static const unsigned int N_PORTS ;
    static const unsigned int N_INPUT_PORTS ;
    static const unsigned int N_OUTPUT_PORTS ;
    static const unsigned int N_TRANSIENT_PEAKS ;
    static const unsigned int DEFAULT_BUFFER_SIZE ;
    static const unsigned int N_BYTES_PER_FRAME ;
    static const unsigned int GUI_UPDATE_IVL ;


    /* JackIO class side private varables */

    // JACK handles
    static jack_client_t* Client ;
#if FIXED_N_AUDIO_PORTS
    static jack_port_t*   InputPort1 ;
    static jack_port_t*   InputPort2 ;
    static jack_port_t*   OutputPort1 ;
    static jack_port_t*   OutputPort2 ;
#else // TODO: much
#endif // #if FIXED_N_AUDIO_PORTS

    // app state
    static Scene*       CurrentScene ;
    static Scene*       NextScene ;

    // audio data
    static unsigned int RecordBufferSize ;
#if FIXED_N_AUDIO_PORTS
    static Sample*      RecordBuffer1 ;
    static Sample*      RecordBuffer2 ;
#  if SCENE_NFRAMES_EDITABLE
/*
    static Sample* LeadInBuffer1 ;
    static Sample* LeadInBuffer2 ;
    static Sample* LeadOutBuffer1 ;
    static Sample* LeadOutBuffer2 ;
*/
#  endif // #if SCENE_NFRAMES_EDITABLE
#else // TODO: much
#endif // #if FIXED_N_AUDIO_PORTS

    // peaks data
    static vector<Sample> PeaksIn ;                       // scope peaks (mono mix)
    static vector<Sample> PeaksOut ;                      // scope peaks (mono mix)
#if FIXED_N_AUDIO_PORTS
    static Sample         TransientPeaks[N_AUDIO_PORTS] ; // VU peaks (per channel)
#else // TODO:
    static Sample         TransientPeaks[N_AUDIO_PORTS] ; // VU peaks (per channel)
#endif // #if FIXED_N_AUDIO_PORTS
    static Sample         TransientPeakInMix ;
//    static Sample         TransientPeakOutMix ;

    // event structs
    static SDL_Event    NewLoopEvent ;
    static unsigned int NewLoopEventSceneN ;
    static Loop*        NewLoopEventLoop ;
    static SDL_Event    SceneChangeEvent ;
    static unsigned int SceneChangeEventSceneN ;

    // metadata
    static jack_nframes_t SampleRate ;
//    static unsigned int       NBytesPerSecond ;
//    static jack_nframes_t     NFramesPerPeriod ;
#if SCENE_NFRAMES_EDITABLE
    static unsigned int   MinLoopSize ;
    static unsigned int   BufferMarginSize ;
    static unsigned int   TriggerLatencySize ;
#  if INIT_JACK_BEFORE_SCENES
    static unsigned int   EndFrameN ;
#  endif // #if INIT_JACK_BEFORE_SCENES
    static unsigned int   BeginFrameN ;
    static unsigned int   BufferMarginsSize ;
    static unsigned int   BytesPerPeriod ;
    static unsigned int   BufferMarginBytes ;

    static unsigned int   TriggerLatencyBytes ;

#else
#  if INIT_JACK_BEFORE_SCENES
    static unsigned int   EndFrameN ;
#  endif // #if INIT_JACK_BEFORE_SCENES
    static unsigned int   BytesPerPeriod ;
#endif // #if SCENE_NFRAMES_EDITABLE
    static unsigned int   FramesPerGuiInterval ;

    // misc flags
    static bool ShouldMonitorInputs ;


  public:

    /* JackIO class side public functions */

    // setup
#if INIT_JACK_BEFORE_SCENES
    static unsigned int Init(bool shouldMonitorInputs , unsigned int recordBufferSize) ;
#else
    static unsigned int Init(Scene* currentScene , bool shouldMonitorInputs ,
                             unsigned int recordBufferSize) ;
#endif // #if INIT_JACK_BEFORE_SCENES
    static void Reset(       Scene* currentScene) ;

    // getters/setters
#if !INIT_JACK_BEFORE_SCENES
    static unsigned int    GetRecordBufferSize(void) ;
#endif // #if !INIT_JACK_BEFORE_SCENES
/*
    static unsigned int    GetNFramesPerPeriod(void) ;
    static unsigned int    GetFrameSize(       void) ;
    static unsigned int    GetSampleRate(      void) ;
    static unsigned int    GetNBytesPerSecond(void) ;
*/
    static void            SetCurrentScene(   Scene* currentScene) ;
    static void            SetNextScene(      Scene* nextScene) ;
    static vector<Sample>* GetPeaksIn(        void) ;
    static vector<Sample>* GetPeaksOut(       void) ;
    static Sample*         GetTransientPeaks( void) ;
    static Sample*         GetTransientPeakIn(void) ;
//    static Sample*         GetTransientPeakOut(   void) ;

    // helpers
    static void   ScanTransientPeaks(void) ;
    static Sample GetPeak(           Sample* buffer , unsigned int nFrames) ;


  private:

    /* JackIO class side private functions */

    // JACK server callbacks
    static int  ProcessCallback(   jack_nframes_t nFramesPerPeriod , void* unused) ;
    static int  SampleRateCallback(jack_nframes_t sampleRate ,       void* unused) ;
    static int  BufferSizeCallback(jack_nframes_t nFramesPerPeriod , void* unused) ;
    static void ShutdownCallback(                                    void* unused) ;

    // helpers
    static jack_port_t* RegisterPort(const char* portName , unsigned long portType) ;
#if SCENE_NFRAMES_EDITABLE
    static void         SetMetaData( jack_nframes_t sampleRate , jack_nframes_t nFramesPerPeriod) ;
#endif // #if SCENE_NFRAMES_EDITABLE
} ;


#endif // #ifndef _JACK_IO_H_


/* NOTE: on RecordBuffer layout

    to allow for dynamic adjustment of seams and compensation for SDL key event delay
      the following are the buffer offsets used:

        invariant                  -->
            BeginFrameN <= beginFrameN <= currentFrameN < endFrameN <= EndFrameN
        initially                  -->
            beginFrameN     = currentFrameN = BeginFrameN = BufferMarginSize
            endFrameN       = EndFrameN     = (RecordBufferSize - BufferSize)
        on initial trigger         -->
            beginFrameN     = currentFrameN - TriggerLatencySize
        on accepting trigger       -->
            if (currentFrameN > beginFrameN + MINIMUM_LOOP_DURATION) // (issue #12)
                endFrameN     = currentFrameN
                nFrames       = endFrameN - TriggerLatencySize - beginFrameN
        after each rollover        -->
            beginFrameN     = BeginFrameN
            endFrameN       = BeginFrameN + nFrames
        after first rollover       -->
            currentFrameN   = BeginFrameN + TriggerLatencySize
        after subsequent rollovers -->
            currentFrameN   = BeginFrameN

    all of the above offsets are multiples of BufferSize (aka nFramesPerPeriod)
      excepting beginFrameN and nFrames

    on each rollover (TODO: this is currently only done when copying base loops)
        set currentFrameN = BeginFrameN (+ TriggerLatencySize if base loop)
        copy tail end of RecordBuffer back to the beginning of RecordBuffer
        this will be the LeadIn of the next loop (may or may not be part of a previous loop)
            let begin = (beginFrameN + nFrames) - BufferMarginSize
        initial base loop -->
            RecordBuffer[begin] upto RecordBuffer[endFrameN]
                --> RecordBuffer[0]
        subsequent loops  -->
            RecordBuffer[begin] upto RecordBuffer[endFrameN + TriggerLatencySize]
                --> RecordBuffer[0]

    on creation of each loop
        copy the entire buffer to include per loop LeadIn
            RecordBuffer[0] upto RecordBuffer[endFrameN]
                --> NewLoop[0]

    on creation of base loops
        set beginFrameN   = BeginFrameN
        set currentFrameN = BeginFrameN + TriggerLatencySize
        set endFrameN     = BeginFrameN + nFrames
        all subsequent loops will use beginFrameN and endFrameN (dynamically) as seams

    on next rollover following creation of each loop
        copy the leading BeginFrameN + BufferMarginSize to the end of the previous loop
          regardless of creating a new loop from the current data
        this will be the leadOut of the previous loop
            RecordBuffer[BeginFrameN] upto RecordBuffer[BeginFrameN + BufferMarginSize]
                --> PrevLoop[BeginFrameN + nFrames]


                            INITIAL RECORD BUFFER
          currentFrameN                                                 // dynamic offset
           beginFrameN                                        endFrameN // dynamic offsets
           BeginFrameN                                        EndFrameN // static offsets
                |                                                 |
 |<-MarginSize->|<--------(RecordBufferSize - MarginSize)-------->|     // sizes
 |<---LeadIn--->|<-----------------RolloverRange----------------->|     // 'zones'
 |<-------------------------RecordBuffer------------------------->|     // buffer


                              INITIAL BASE LOOP
                 beginFrameN                       endFrameN                  // offsets
thisLeadInBegin       |                                |                      // pointer
       |              |                                |
 |<-?->|<-MarginSize->|<-----------nFrames------------>|<-MarginSize->|<-?->| // sizes
 |     |<---LeadIn--->|<--------RolloverRange--------->|<--LeadOut--->|     | // 'zones'
 |     |<-------------------Source-------------------->|              |     | // data
 |     |<--------------------Dest--------------------->|<--Pending--->|     | // data
 |     |<--------------------------NewLoop--------------------------->|     | // dest
 |<------------------------------RecordBuffer------------------------------>| // source


                               SUBSEQUENT LOOPS
           beginFrameN                       endFrameN                        // offsets
thisLeadInBegin |                                |                            // pointer
 |              |                                |
 |<-MarginSize->|<-----------nFrames------------>|<-MarginSize->|<----?---->| // sizes
 |<---LeadIn--->|<--------RolloverRange--------->|<--LeadOut--->|           | // 'zones'
 |<-------------------Source-------------------->|              |           | // data
 |<--------------------Dest--------------------->|<--Pending--->|           | // data
 |<--------------------------NewLoop--------------------------->|           | // dest
 |<------------------------------RecordBuffer------------------------------>| // source


                             SHIFT NEXT LEAD-IN
           beginFrameN                      endFrameN                         // offsets
                |         nextLeadInBegin       |                             // pointer
                |                |              |
 |<-MarginSize->|                |<-MarginSize->|                           | // sizes
 |<----Dest---->|                |<---Source--->|                           | // data
 |<------------------------------RecordBuffer------------------------------>| // source/dest


                        COPY PREVIOUS LEAD_OUT (TODO)
           beginFrameN                       endFrameN                        // offsets
                |           mixBegin             |                            // pointer
                |              |                 |
 |<-MarginSize->|<-MarginSize->|                 |<-MarginSize->|           | // sizes
 |              |<---Source--->|                 |<----Dest---->|           | // data
 |<---LeadIn--->|<--------RolloverRange--------->|<--LeadOut--->|           | // 'zones'
 |<--------------------------NewLoop--------------------------->|           | // dest
 |<------------------------------RecordBuffer------------------------------>| // source
*/
