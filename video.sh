#!/bin/bash
CONF="-c h264_nvenc -b:v 10000k -c:a copy"
# CONF="-c libx264 -b 64k"

# https://stackoverflow.com/questions/17623676/text-on-video-ffmpeg
# x=(w-text_w)/2: y=(h-text_h)/2

TXT_CONF='fontcolor=white: fontsize=72: box=1: boxcolor=black@0.5: boxborderw=5'

io() {
    for i in `seq $2`; do
        echo -n "[$1$i] "
    done
}

conv_time() {
    python3 -c 'import sys; a=sys.argv[1]; print(60*int(a[0:2])+int(a[2:4]))' $1
}

speed_up() {
    echo "[t$1] trim=`conv_time $2`:`conv_time $3`, setpts=PTS*$4,
         setpts=PTS-STARTPTS, drawtext=text=$5: $TXT_CONF: x=100: y=h-150
         [o$1]"
}

INPUT="IMG_3472.MOV"
M=7

FILTER="movie='$INPUT', split=$M `io t $M`;
    `speed_up  1 0122 0300 1    ""`;
    `speed_up  2 0300 0630 0.25 "4x Speed Up"`;
    `speed_up  3 0630 0655 1    ""`;
    `speed_up  4 0655 0840 0.25 "4x Speed Up"`;
    `speed_up  5 0840 0947 1    ""`;
    `speed_up  6 0958 1028 1    ""`;
    `speed_up  7 1111 1158 1    ""`;
    `io o $M` concat=n=$M [out]
"

# ffplay -f lavfi "$FILTER; [out] split=1 [out0]"
ffmpeg -filter_complex "$FILTER" -i audio.aac -map '[out]' -map '0' $CONF \
    video.mp4

# 0122 - 0218
# 0238 - 0243
# 0250 - 0852
#     0635
# 0852 - 0921
# 0921 - 0947
# 
# 0958 - 1028
# 
# 1111 - 1158

