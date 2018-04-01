g++ -w combine_images_to_video.cpp -std=c++0x -I/usr/local/include/opencv2 -L/usr/local/lib -L/usr/local/cuda/lib64/ `pkg-config --cflags --libs opencv`   -lavutil -lavcodec -lavformat  \
                   -lavfilter -ldl -lasound -L/usr/lib  -lpthread -lz -lswscale -lm  -o video_from_combined_images
