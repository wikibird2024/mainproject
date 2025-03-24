#!/bin/bash

# Định nghĩa thư mục dự án (đã có sẵn)
PROJECT_DIR="$HOME/esp/mainproject"

# Danh sách các component cần tạo
COMPONENTS=("comm" "buzzer" "mpu6050" "sim4g_gps")

# Kiểm tra thư mục dự án đã tồn tại chưa
if [ ! -d "$PROJECT_DIR" ]; then
    echo "❗ Thư mục $PROJECT_DIR không tồn tại! Hãy tạo trước khi chạy script."
    exit 1
fi

echo "🚀 Đang thiết lập cấu trúc cho dự án tại: $PROJECT_DIR"

# Tạo thư mục components nếu chưa có
mkdir -p "$PROJECT_DIR/components"

# Tạo các component
for COMPONENT in "${COMPONENTS[@]}"; do
    COMPONENT_DIR="$PROJECT_DIR/components/$COMPONENT"
    mkdir -p "$COMPONENT_DIR"
    
    # Tạo file source & header nếu chưa có
    if [ ! -f "$COMPONENT_DIR/$COMPONENT.c" ]; then
        cat <<EOF > "$COMPONENT_DIR/$COMPONENT.c"
#include "$COMPONENT.h"
#include "esp_log.h"

static const char *TAG = "$COMPONENT";

void ${COMPONENT}_init(void) {
    ESP_LOGI(TAG, "$COMPONENT initialized");
}
EOF
    fi

    if [ ! -f "$COMPONENT_DIR/$COMPONENT.h" ]; then
        cat <<EOF > "$COMPONENT_DIR/$COMPONENT.h"
#pragma once
void ${COMPONENT}_init(void);
EOF
    fi

    # CMakeLists.txt cho component
    if [ ! -f "$COMPONENT_DIR/CMakeLists.txt" ]; then
        cat <<EOF > "$COMPONENT_DIR/CMakeLists.txt"
idf_component_register(SRCS "$COMPONENT.c"
                       INCLUDE_DIRS ".")
EOF
    fi

    echo "✅ Component $COMPONENT đã được tạo"
done

# Tạo CMakeLists.txt chính nếu chưa có
if [ ! -f "$PROJECT_DIR/CMakeLists.txt" ]; then
    cat <<EOF > "$PROJECT_DIR/CMakeLists.txt"
cmake_minimum_required(VERSION 3.16)
include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mainproject)
EOF
    echo "✅ CMakeLists.txt đã được tạo"
fi

# Tạo thư mục main và file main.c nếu chưa có
mkdir -p "$PROJECT_DIR/main"

if [ ! -f "$PROJECT_DIR/main/main.c" ]; then
    cat <<EOF > "$PROJECT_DIR/main/main.c"
#include <stdio.h>
#include "comm.h"
#include "buzzer.h"
#include "mpu6050.h"
#include "sim4g_gps.h"

void app_main(void) {
    printf("🚀 ESP32 Fall Alert System Started\n");
    
    comm_init();
    buzzer_init();
    mpu6050_init();
    sim4g_gps_init();
}
EOF
    echo "✅ File main.c đã được tạo"
fi

echo "🎉 Dự án đã được thiết lập hoàn tất tại: $PROJECT_DIR"
echo "➡ Hãy vào thư mục dự án và build bằng: cd $PROJECT_DIR && idf.py build"

