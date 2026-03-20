#!/bin/bash
export OPENCV_IO_ENABLE_OPENEXR=0
export MUJOCO_GL=egl
export LIBGL_ALWAYS_SOFTWARE=1
export GALLIUM_DRIVER=llvmpipe
export TF_CPP_MIN_LOG_LEVEL=2

echo "Запуск распознавания лиц в виртуальном X сервере..."
xvfb-run -a python3.11 /home/user/Desktop/8_sem/nikitenkova/Face_Recognition/_2/_2.py