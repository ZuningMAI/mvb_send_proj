#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
from datetime import datetime
import argparse


def parse_log_fragmented_frames(log_file):
    """
    解析日志中的分段帧数据行
    格式示例:
    [2025-11-11 23:23:28.766] [FRAME] PD=0XA0 | 数据: 分段1/2: 7e 21 04 00 a0 00 00 26 50 00 06 00 13 00 0d 00 0c 10 69 00 | 发送: 20/20 字节
    [2025-11-11 23:23:28.779] [FRAME] PD=---- | 数据: 分段2/2(续): 00 08 70 00 5f 0b f4 02 96 00 00 00 00 00 00 00 00 73 68 7e | 发送: 20/20 字节
    """
    frames = []
    first_segments = []  # 存储分段帧的第一部分
    
    with open(log_file, 'r', encoding='utf-8') as f:
        for line in f:
            # 检查是否是帧数据行
            if '[FRAME] PD=' in line and '数据:' in line:
                try:
                    # 提取时间戳
                    timestamp_start = line.find('[') + 1
                    timestamp_end = line.find(']')
                    timestamp_str = line[timestamp_start:timestamp_end]
                    timestamp = datetime.strptime(timestamp_str, '%Y-%m-%d %H:%M:%S.%f')
                    
                    # 提取PD类型
                    pd_start = line.find('PD=') + 3
                    pd_end = line.find(' ', pd_start)
                    if pd_end == -1:
                        pd_end = line.find(' |', pd_start)
                    if pd_end == -1:
                        pd_end = len(line)
                    pd_type = line[pd_start:pd_end].strip()
                    
                    # 提取数据
                    data_start = line.find('数据:') + 3
                    data_end = line.find(' |', data_start)
                    if data_end == -1:
                        data_end = len(line)
                    data_part = line[data_start:data_end].strip()
                    
                    # 检查是否是分段帧
                    if '分段' in data_part and '/' in data_part:
                        # 提取分段信息
                        segment_info = data_part.split(':')[0]  # "分段1/2" 或 "分段2/2(续)"
                        segment_num = int(segment_info[2])  # 1 or 2
                        
                        # 提取十六进制数据
                        hex_start = data_part.find(': ') + 2
                        hex_data = data_part[hex_start:].replace(' ', '').upper()
                        
                        if segment_num == 1:
                            # 保存第一部分分段帧
                            first_segments.append({
                                'timestamp': timestamp,
                                'pd_type': pd_type,
                                'segment_data': hex_data
                            })
                        elif segment_num == 2 and pd_type == '----':
                            # 处理第二部分分段帧
                            if first_segments:
                                # 获取最近的第一部分帧
                                first_segment = first_segments.pop()
                                # 合并两个分段 (直接连接，不需要去除7E)
                                full_data = first_segment['segment_data'] + hex_data
                                
                                frames.append({
                                    'timestamp': first_segment['timestamp'],
                                    'pd_type': first_segment['pd_type'],
                                    'data': full_data
                                })
                    else:
                        # 不是分段帧，直接处理
                        # 提取十六进制数据（非分段情况）
                        hex_start = data_part.find(': ') + 2
                        if hex_start > 1:  # 说明找到了": "
                            hex_data = data_part[hex_start:].replace(' ', '').upper()
                        else:
                            # 直接处理数据部分
                            hex_data = data_part.replace(' ', '').upper()
                            
                        frames.append({
                            'timestamp': timestamp,
                            'pd_type': pd_type,
                            'data': hex_data
                        })
                except Exception as e:
                    # 解析出错，跳过该行
                    continue
    
    # 处理剩余未完成的分段帧（如果有）
    for segment in first_segments:
        frames.append({
            'timestamp': segment['timestamp'],
            'pd_type': segment['pd_type'],
            'data': segment['segment_data']
        })
    
    return frames


def parse_txt_frames(txt_file):
    """
    解析txt文件中的帧数据行
    格式: [23:23:28.787] PD:0XA0 原始数据:7E 21 04 00 A0 00 00 26 50 00 06 00 13 00 0D 00 0C 10 69 00 00 08 70 00 5F 0B F4 02 96 00 00 00 00 00 00 00 00 73 68 7E
    """
    frames = []
    with open(txt_file, 'r', encoding='utf-8') as f:
        for line in f:
            # 如果是帧数据行
            if 'PD:' in line and '原始数据:' in line:
                try:
                    # 提取时间
                    time_start = line.find('[') + 1
                    time_end = line.find(']')
                    time_str = line[time_start:time_end]
                    
                    # 提取PD类型
                    pd_start = line.find('PD:') + 3
                    pd_end = line.find(' ', pd_start)
                    if pd_end == -1:
                        pd_end = line.find('原始数据', pd_start)
                    if pd_end == -1:
                        pd_end = len(line)
                    pd_type = line[pd_start:pd_end].strip()
                    
                    # 提取数据
                    data_start = line.find('原始数据:') + 5
                    data_hex = line[data_start:].strip().replace(' ', '')
                    
                    timestamp = datetime.strptime(time_str, '%H:%M:%S.%f')
                    frames.append({
                        'time': timestamp.time(),
                        'pd_type': pd_type,
                        'data': data_hex
                    })
                except Exception as e:
                    # 解析出错，跳过该行
                    continue
    return frames


def extract_frames_from_log(log_file):
    """
    从.log文件中提取所有帧数据（兼容分段帧）
    """
    return parse_log_fragmented_frames(log_file)


def extract_frames_from_txt(txt_file):
    """
    从data.txt文件中提取所有帧数据
    """
    return parse_txt_frames(txt_file)


def match_frames_by_pd(log_frames, txt_frames):
    """
    按PD类型对帧进行分组
    """
    log_by_pd = {}
    txt_by_pd = {}
    
    # 按PD类型分组log帧
    for frame in log_frames:
        pd_type = frame['pd_type']
        if pd_type not in log_by_pd:
            log_by_pd[pd_type] = []
        log_by_pd[pd_type].append(frame)
    
    # 按PD类型分组txt帧
    for frame in txt_frames:
        pd_type = frame['pd_type']
        if pd_type not in txt_by_pd:
            txt_by_pd[pd_type] = []
        txt_by_pd[pd_type].append(frame)
        
    return log_by_pd, txt_by_pd


def compare_frames(log_frames, txt_frames, time_tolerance=1.0):
    """
    比较两个帧列表中的数据
    time_tolerance: 时间容忍度（秒）
    """
    matched_pairs = []
    unmatched_log = []
    unmatched_txt = []
    
    # 创建txt帧的时间索引用于快速查找
    txt_time_index = {}
    for frame in txt_frames:
        time_key = frame['time']
        pd_type = frame['pd_type']
        if pd_type not in txt_time_index:
            txt_time_index[pd_type] = []
        txt_time_index[pd_type].append(frame)
    
    # 尝试匹配log帧和txt帧
    for log_frame in log_frames:
        pd_type = log_frame['pd_type']
        log_time = log_frame['timestamp']
        log_data = log_frame['data']
        
        matched = False
        # 查找相同PD类型的txt帧
        if pd_type in txt_time_index:
            for txt_frame in txt_time_index[pd_type][:]:  # 使用[:]创建副本以便安全删除
                txt_time = txt_frame['time']
                txt_data = txt_frame['data']
                
                # 比较时间（只比较时分秒）
                time_diff = abs(
                    (log_time.hour * 3600 + log_time.minute * 60 + log_time.second + log_time.microsecond / 1000000) -
                    (txt_time.hour * 3600 + txt_time.minute * 60 + txt_time.second + txt_time.microsecond / 1000000)
                )
                
                # 如果时间接近且数据相同，则匹配成功
                if time_diff <= time_tolerance and log_data == txt_data:
                    matched_pairs.append({
                        'log_frame': log_frame,
                        'txt_frame': txt_frame,
                        'time_diff': time_diff
                    })
                    matched = True
                    # 从索引中移除已匹配的帧，避免重复匹配
                    txt_time_index[pd_type].remove(txt_frame)
                    break
        
        if not matched:
            unmatched_log.append(log_frame)
    
    # 剩余未匹配的txt帧
    for pd_type, frames in txt_time_index.items():
        unmatched_txt.extend(frames)
    
    return matched_pairs, unmatched_log, unmatched_txt


def main():
    parser = argparse.ArgumentParser(description='比较.log文件和data.txt中的MVB分段帧数据')
    parser.add_argument('--log', default='bin/Log/XX1.log', help='日志文件路径')
    parser.add_argument('--data-dir', default='dataXX1/originData/2025-11-12', help='data.txt文件目录')
    
    args = parser.parse_args()
    
    # 检查日志文件是否存在
    if not os.path.exists(args.log):
        print(f"日志文件不存在: {args.log}")
        return
    
    # 提取log文件中的帧
    print(f"正在解析日志文件: {args.log}")
    log_frames = extract_frames_from_log(args.log)
    print(f"从日志文件中提取到 {len(log_frames)} 帧数据")
    
    # 提取txt文件中的帧
    txt_frames = []
    data_dirs = []
    if os.path.exists(args.data_dir):
        for item in os.listdir(args.data_dir):
            if os.path.isdir(os.path.join(args.data_dir, item)):
                data_dirs.append(item)
    
    for pd_dir in data_dirs:
        txt_file = os.path.join(args.data_dir, pd_dir, 'data.txt')
        if os.path.exists(txt_file):
            print(f"正在解析数据文件: {txt_file}")
            frames = extract_frames_from_txt(txt_file)
            txt_frames.extend(frames)
            print(f"从 {txt_file} 中提取到 {len(frames)} 帧数据")
    
    print(f"总共从数据文件中提取到 {len(txt_frames)} 帧数据")
    
    # 按PD类型分组
    log_by_pd, txt_by_pd = match_frames_by_pd(log_frames, txt_frames)
    
    # 分别比较每种PD类型的数据
    total_matched = 0
    total_unmatched_log = 0
    total_unmatched_txt = 0
    
    all_pd_types = set(list(log_by_pd.keys()) + list(txt_by_pd.keys()))
    for pd_type in sorted(all_pd_types):
        print(f"\n=== 比较 PD 类型 {pd_type} ===")
        pd_log_frames = log_by_pd.get(pd_type, [])
        pd_txt_frames = txt_by_pd.get(pd_type, [])
        
        print(f"日志中有 {len(pd_log_frames)} 帧, 数据文件中有 {len(pd_txt_frames)} 帧")
        
        matched_pairs, unmatched_log, unmatched_txt = compare_frames(pd_log_frames, pd_txt_frames)
        
        print(f"匹配成功: {len(matched_pairs)} 帧")
        print(f"日志中未匹配: {len(unmatched_log)} 帧")
        print(f"数据文件中未匹配: {len(unmatched_txt)} 帧")
        
        total_matched += len(matched_pairs)
        total_unmatched_log += len(unmatched_log)
        total_unmatched_txt += len(unmatched_txt)
        
        # 显示未匹配的帧详情
        if unmatched_log:
            print(f"\n{pd_type} 类型在日志中未匹配的帧:")
            for frame in unmatched_log[:5]:  # 只显示前5个
                print(f"  时间: {frame['timestamp']}, 数据长度: {len(frame['data'])}")
            if len(unmatched_log) > 5:
                print(f"  ... 还有 {len(unmatched_log) - 5} 帧未显示")
        
        if unmatched_txt:
            print(f"\n{pd_type} 类型在数据文件中未匹配的帧:")
            for frame in unmatched_txt[:5]:  # 只显示前5个
                print(f"  时间: {frame['time']}, 数据长度: {len(frame['data'])}")
            if len(unmatched_txt) > 5:
                print(f"  ... 还有 {len(unmatched_txt) - 5} 帧未显示")
    
    print(f"\n=== 总结 ===")
    print(f"总匹配帧数: {total_matched}")
    print(f"日志中未匹配帧数: {total_unmatched_log}")
    print(f"数据文件中未匹配帧数: {total_unmatched_txt}")
    
    if total_unmatched_log == 0 and total_unmatched_txt == 0:
        print("恭喜！所有字段都匹配。")
    else:
        print("存在不匹配的帧，请检查以上详细信息。")


if __name__ == '__main__':
    main()