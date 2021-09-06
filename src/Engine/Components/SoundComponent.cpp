#include <Engine/Components/SoundComponent.hpp>

#include <cstdio>
#include <cassert>
#include <string>
#include <iostream>

namespace Engine
{

    uint32_t SoundComponent::instanceCount = 0;
    
    SoundComponent::SoundComponent()
    : mLoaded(false)
    , mLoopFlag(false)
    , mPlayFlag(false)
    , mPlayingDuration(0)
    , mRIFFFileSize(0)
    , mPCMDataSize(0)
    , mBufPos(0)
    , mVolumeRate(0.3f)
    {
        if(!instanceCount)
        {
            Pa_Initialize();
            ++instanceCount;
        }

    }

    SoundComponent::~SoundComponent()
    {
        --instanceCount;
        if(!instanceCount)
            Pa_Terminate();
    }

    void SoundComponent::setVolumeRate(float rate)
    {
        mVolumeRate = rate;
    }

    float SoundComponent::getVolumeRate() const
    {
        return mVolumeRate;
    }

    const SoundComponent::WaveFormat& SoundComponent::getWaveFormat() const
    {
        return mFormat;
    }

    bool SoundComponent::getEndFlag()
    {
        if(!mLoaded)
            return true;
        
        if(mFormat.bits_per_sample == 8)
        {
            return mBufPos >= (int)data_8bit.size();
        }
        else if(mFormat.bits_per_sample == 16)
        {
            return mBufPos >= (int)data_16bit.size();
        }
    
        return true;
    }

    void SoundComponent::setLoop(bool flag)
    {
        mLoopFlag = flag;
    }

    bool SoundComponent::getLoopFlag() const
    {
        return mLoopFlag;
    }

    // std::weak_ptr<PaStream> SoundComponent::getStream()
    // {
    //     return mStream;
    // }

    double SoundComponent::getPlayingTime() const
    {
        return mPlayingDuration;
    }

    void SoundComponent::play()
    {
        if(!mLoaded || !mpStream)
        {
            assert(!"did not loaded data!");
            return;
        }

        mPlayFlag = true;
        Pa_StartStream(mpStream.value());
        mStartTime = std::chrono::high_resolution_clock::now();
    }

    void SoundComponent::stop()
    {
        if(!mLoaded || !mpStream)
        {
            assert(!"did not loaded data!");
            return;
        }

        mPlayFlag = false;
        Pa_StopStream(mpStream.value());
    }

    void SoundComponent::update()
    {
        if(mPlayFlag)
        {
            auto&& now = std::chrono::high_resolution_clock::now();
            mPlayingDuration = std::chrono::duration_cast<std::chrono::microseconds>(now - mStartTime).count() / 1000000.;
        }
    }

    void SoundComponent::create(const char* pathToWavFile, bool loop)
    {
        if(mLoaded)
        {
            if(mpStream)
            {
                Pa_StopStream(mpStream.value());
                Pa_CloseStream(mpStream.value());
            }
        }

        FILE *fp;
        fp = fopen(pathToWavFile, "rb");

        if(!fp)
        {
            assert(!"failed to open file!");
            return;
        }

        std::string readbuf;    //適当
        readbuf.resize(4);
        int readbuf2;   //適当なバッファ

        //RIFFを読む
        fread((char*)readbuf.c_str(), 4, 1, fp); //4byte読む　"RIFF"がかえる
        if(readbuf != "RIFF")
        {
            assert(!"failed to load!");
            return;
        }

        fread(&mRIFFFileSize, 4, 1, fp);  //4byte読む　これ以降のファイルサイズ (ファイルサイズ - 8)が返る
        //WAVEチャンク 
        fread((char*)readbuf.c_str(), 4,1,fp); //4byte読む　"WAVE"がかえる
        if(readbuf != "WAVE")
        {
            assert(!"failed to load!");
            return;
        }

        //フォーマット定義
        fread((char*)readbuf.c_str(), 4,1,fp); //4byte読む　"fmt "がかえる
        if(readbuf != "fmt ")
        {
            assert(!"failed to load!");
            return;
        }

        //mFormatチャンクのバイト数
        fread(&readbuf2, 4, 1, fp);  //4byte読む　リニアPCMならば16が返る

        //データ1
        fread(&mFormat.format_id,       2, 1, fp); //2byte読む　リニアPCMならば1が返る
        fread(&mFormat.channel,         2, 1, fp); //2byte読む　モノラルならば1が、ステレオならば2が返る
        fread(&mFormat.sampling_rate,   4, 1, fp); //4byte読む　44.1kHzならば44100が返る
        fread(&mFormat.bytes_per_sec,   4, 1, fp); //4byte読む　44.1kHz 16bit ステレオならば44100×2×2 = 176400
        fread(&mFormat.block_size,      2, 1, fp); //2byte読む　16bit ステレオならば2×2 = 4
        fread(&mFormat.bits_per_sample, 2, 1, fp); //2byte読む　16bitならば16

        //拡張部分は存在しないものとして扱う
        if(mFormat.format_id != 1)
        {
            assert(!"failed to load!");
            return;
        }

        //dataチャンク
        fread((char*)readbuf.c_str(), 4, 1, fp); //4byte読む　"data"がかえる
        if(readbuf != "data")
        {
            assert(!"failed to load!");
            return;
        }

        fread(&mPCMDataSize, 4, 1, fp);   //4byte読む　実効データのバイト数がかえる

        //以下、上のデータサイズ分だけブロックサイズ単位でデータ読み出し
        if(mFormat.bits_per_sample == 8)
        {
            //8ビット
            data_8bit.resize(mPCMDataSize+1);    //reserveだと範囲外エラー
            fread(&data_8bit[0], 1, (size_t)2 * mPCMDataSize / mFormat.block_size,fp);    //vectorに1byteずつ詰めていく
        }
        else if(mFormat.bits_per_sample == 16)
        {
            //16ビット
            data_16bit.resize(mPCMDataSize/2+2); //reserveだと範囲外エラー
            fread(&data_16bit[0], 2, (size_t)2 * mPCMDataSize / mFormat.block_size,fp);   //vectorに2byteずつ詰めていく
        }

        fclose(fp);

        //pastream作成---------------------------------------
        int (*callback)(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData) = 
        [](
            const void *inputBuffer,
            void *outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void *userData )->int
        {
            SoundComponent* data = (SoundComponent*)userData;   //userDataをWAVE*型へキャストしておく
            float* out = (float*)outputBuffer;    //同じくoutputもキャスト
            (void) timeInfo;    
            (void) statusFlags; //この辺はよく分からんけど使わないらしい（未調査) 

            //フレーム単位で回す
            for(int i=0; i < (int)framesPerBuffer; i++ )
            {
                //インターリーブ方式でL,R,L,R,...と記録されてる（WAVも同じ）ので、
                //outputに同じ感じで受け渡してやる
                for(int c=0 ; c < (int)data->getWaveFormat().channel ; ++c)
                {
                    //チャンネル数分だけ回す
                    //残りの音声データがなく、ループフラグが立ってない場合終了
                    if(data->getEndFlag() && !data->getLoopFlag())
                    {
                        return (int)paComplete;
                    }
                    *out++ = data->getVolumeRate() * (data->readData<float>())/32767.0f; //-1.0～1.0に正規化(内部ではshortで保持してるので,floatに変換)
                }
            }

            return paContinue;
        };

        PaStreamParameters outputParameters; 
        {
            //アウトプットデバイスの構築(マイク使わない場合はインプットデバイス使わない)
            outputParameters.device = Pa_GetDefaultOutputDevice();  //デフォデバイスを使う
            outputParameters.channelCount = mFormat.channel;    //フォーマット情報を使う
            outputParameters.sampleFormat = paFloat32; //paFloat32(PortAudio組み込み)型
            outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device)->defaultLowOutputLatency;   //レイテンシの設定
            outputParameters.hostApiSpecificStreamInfo = NULL;  //よくわからん
        }

        PaStream* stream;
        auto err = Pa_OpenStream(
            &stream,            //なんか再生するべきストリーム情報が帰ってくる
            NULL,               //マイクとかのインプットないのでNULL
            &outputParameters,  //アウトプット設定
            44100,//mFormat.sampling_rate,  //44100Hz
            paFramesPerBufferUnspecified,//1サンプルあたりのバッファ長(自動)
            paNoFlag,           //ストリーミングの設定らしい　とりあえず0
            callback,           //コールバック関数（上のラムダ）
            this);            //wavファイルデータを渡す（ちなみにどんなデータでも渡せる）
        
        mpStream = stream;

        if( err != paNoError ) 
        { //エラー処理
            Pa_Terminate();
            std::cerr << "error : " << err << "\n";
            std::cerr << mFormat.format_id << "\n";
            std::cerr << mFormat.channel << "\n";
            std::cerr << mFormat.sampling_rate << "\n";
            std::cerr << mFormat.bytes_per_sec << "\n";
            std::cerr << mFormat.block_size << "\n";
            std::cerr << mFormat.bits_per_sample << "\n";
            assert(!"failed to open stream!");
            return;
        }

        mLoaded = true;
        mLoopFlag = loop;
    }
}