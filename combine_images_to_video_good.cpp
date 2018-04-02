#include<iostream>
#include <vector>


extern "C"
{
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}
#define STRIDE [1920]
int NUMNUMBER=3;
int W_VIDEO = 640;
int H_VIDEO = 480;
AVFrame* OpenImage(const char* imageFileName)
{
    AVFormatContext *pFormatCtx =  avformat_alloc_context();
    std::cout<<"1"<<imageFileName<<std::endl;
    if( avformat_open_input(&pFormatCtx, imageFileName, NULL, NULL) < 0)
    {
        printf("Can't open image file '%s'\n", imageFileName);
        return NULL;
    }       
    std::cout<<"2"<<std::endl;
    av_dump_format(pFormatCtx, 0, imageFileName, false);

    AVCodecContext *pCodecCtx;
    std::cout<<"3"<<std::endl;
    pCodecCtx = pFormatCtx->streams[0]->codec;
    pCodecCtx->width = W_VIDEO;
    pCodecCtx->height = H_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    // Find the decoder for the video stream
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (!pCodec)
    {
        printf("Codec not found\n");
        return NULL;
    }

    // Open codec
    //if(avcodec_open2(pCodecCtx, pCodec)<0)
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)//check this NULL, it should be of AVDictionary **options
    {
        printf("Could not open codec\n");
        return NULL;
    }
    std::cout<<"4"<<std::endl;
    // 
    AVFrame *pFrame;

    pFrame = av_frame_alloc();

    if (!pFrame)
    {
        printf("Can't allocate memory for AVFrame\n");
        return NULL;
    }
    printf("here");
    int frameFinished;
    int numBytes;

    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    avpicture_fill((AVPicture *) pFrame, buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

    // Read frame

    AVPacket packet;

    int framesNumber = 0;
    while (av_read_frame(pFormatCtx, &packet) >= 0)
    {
        if(packet.stream_index != 0)
            continue;

        int ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
        if (ret > 0)
        {
            printf("Frame is decoded, size %d\n", ret);
            pFrame->quality = 4;
            return pFrame;
        }
        else
            printf("Error [%d] while decoding frame: %s\n", ret, strerror(AVERROR(ret)));
    }
}
int main(int argc, char **argv)
{
    if (argc != 6) {
        std::cout << "Usage: cv2ff <dir_name> <prefix> <image surname> <total frames> <outfile>" << std::endl;
        return 1;
    }

    const char* infile_dir = argv[1];
    const char* infile_prefix = argv[2];
    const char* infile_surname = argv[3];
    const int total_frames = atoi(argv[4]);

    if (total_frames <= 0){
        std::cout << "Usage: cv2ff <dir_name> <prefix> <image surname> <total frames> <outfile>" << std::endl;
        std::cout << "Please check that the 4th argument is integer value of total frames"<<std::endl;
        return 1;
    }
    printf("max %d frames\n",total_frames);
    const char* outfile = argv[5];
    char *imageFileName;
    char numberChar[NUMNUMBER];
    // initialize FFmpeg library
    av_register_all();
    //  av_log_set_level(AV_LOG_DEBUG);
    int ret;

    const int dst_width = W_VIDEO;
    const int dst_height = H_VIDEO;
    const AVRational dst_fps = {30, 1};//{fps,1}
    std::cout<<"Hello World"<<std::endl;

   
   
    // open output format context
    AVFormatContext* outctx = nullptr;
    ret = avformat_alloc_output_context2(&outctx, nullptr, nullptr, outfile);
    if (ret < 0) {
        std::cerr << "fail to avformat_alloc_output_context2(" << outfile << "): ret=" << ret;
        return 2;
    }

    // open output IO context
    ret = avio_open2(&outctx->pb, outfile, AVIO_FLAG_WRITE, nullptr, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avio_open2: ret=" << ret;
        return 2;
    }
// create new video stream
    AVCodec* vcodec = avcodec_find_encoder(outctx->oformat->video_codec);
    AVStream* vstrm = avformat_new_stream(outctx, vcodec);
    if (!vstrm) {
        std::cerr << "fail to avformat_new_stream";
        return 2;
    }
    avcodec_get_context_defaults3(vstrm->codec, vcodec);
    vstrm->codec->width = dst_width;
    vstrm->codec->height = dst_height;
    vstrm->codec->pix_fmt = vcodec->pix_fmts[0];
    vstrm->codec->time_base = vstrm->time_base = av_inv_q(dst_fps);
    vstrm->r_frame_rate = vstrm->avg_frame_rate = dst_fps;
    if (outctx->oformat->flags & AVFMT_GLOBALHEADER)
        vstrm->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // open video encoder
    ret = avcodec_open2(vstrm->codec, vcodec, nullptr);
    if (ret < 0) {
        std::cerr << "fail to avcodec_open2: ret=" << ret;
        return 2;
    }

    std::cout
        << "outfile: " << outfile << "\n"
        << "format:  " << outctx->oformat->name << "\n"
        << "vcodec:  " << vcodec->name << "\n"
        << "size:    " << dst_width << 'x' << dst_height << "\n"
        << "fps:     " << av_q2d(dst_fps) << "\n"
        << "pixfmt:  " << av_get_pix_fmt_name(vstrm->codec->pix_fmt) << "\n"
        << std::flush;

    // initialize sample scaler
    SwsContext* swsctx = sws_getCachedContext(
        nullptr, dst_width, dst_height, AV_PIX_FMT_BGR24,
        dst_width, dst_height, vstrm->codec->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsctx) {
        std::cerr << "fail to sws_getCachedContext";
        return 2;
    }
   // allocate frame buffer for encoding
    AVFrame* frame = av_frame_alloc();
    std::vector<uint8_t> framebuf(avpicture_get_size(vstrm->codec->pix_fmt, dst_width, dst_height));
    avpicture_fill(reinterpret_cast<AVPicture*>(frame), framebuf.data(), vstrm->codec->pix_fmt, dst_width, dst_height);
    frame->width = dst_width;
    frame->height = dst_height;
    frame->format = static_cast<int>(vstrm->codec->pix_fmt);

    // encoding loop
    avformat_write_header(outctx, nullptr);
    int64_t frame_pts = 0;
    unsigned nb_frames = 0;
    bool end_of_stream = false;
    int got_pkt = 0;
    int i =0;
    imageFileName = (char *)malloc(strlen(infile_dir)+1+strlen(infile_prefix)+NUMNUMBER+1+strlen(infile_surname)+1);
    do{
        if(!end_of_stream){
        
        strcpy(imageFileName,infile_dir);
        strcat(imageFileName,"/");
        strcat(imageFileName,infile_prefix);
        sprintf(numberChar,"%03d",i+1);
        strcat(imageFileName,numberChar);
        strcat(imageFileName,".");
        strcat(imageFileName,infile_surname);

        std::cout<<imageFileName<<std::endl;


        AVFrame* frame_from_file =  OpenImage(imageFileName);
        if(!frame_from_file){
           std::cout<<"error OpenImage"<<std::endl;
           break;
        }
        sws_scale(swsctx, frame_from_file->data, STRIDE , 0, frame_from_file->height, frame->data, frame->linesize);
        frame->pts = frame_pts++;
        av_frame_free(&frame_from_file);
        }
        // encode video frame
        AVPacket pkt;
        pkt.data = nullptr;
        pkt.size = 0;
        av_init_packet(&pkt);
        ret = avcodec_encode_video2(vstrm->codec, &pkt, end_of_stream ? nullptr : frame, &got_pkt);
        if (ret < 0) {
            std::cerr << "fail to avcodec_encode_video2: ret=" << ret << "\n";
            break;
        }
        if (true) {
            // rescale packet timestamp
            pkt.duration = 1;
            av_packet_rescale_ts(&pkt, vstrm->codec->time_base, vstrm->time_base);
            // write packet
            av_write_frame(outctx, &pkt);
            std::cout << nb_frames << '\r' << std::flush;  // dump progress
            ++nb_frames;
        }
        av_free_packet(&pkt);
        i++;
        //if(i==180)
          // end_of_stream = true;
    } while (i<total_frames);

    av_write_trailer(outctx);
    std::cout << nb_frames << " frames encoded" << std::endl;

    av_frame_free(&frame);
    avcodec_close(vstrm->codec);
    avio_close(outctx->pb);
    avformat_free_context(outctx);
    free(imageFileName);
 

   return 0;
}
