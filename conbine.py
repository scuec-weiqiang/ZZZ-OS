#!/usr/bin/env python
# coding=utf-8
'''
FilePath: /ZZZ/conbine.py
Description:  
Author: scuec_weiqiang scuec_weiqiang@qq.com
Date: 2025-05-02 23:10:23
LastEditTime: 2025-05-02 23:11:02
LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
'''
import os

def combine_code_files(root_dir, output_file='combined_code.txt'):
    # 支持的扩展名集合
    valid_extensions = {'.c', '.h', '.S','.ld'}
    
    with open(output_file, 'w', encoding='utf-8') as outfile:
        for root, dirs, files in os.walk(root_dir):
            for file in files:
                # 获取文件扩展名并转换为小写
                ext = os.path.splitext(file)[1].lower()
                if ext in valid_extensions:
                    file_path = os.path.join(root, file)
                    try:
                        with open(file_path, 'r', encoding='utf-8', errors='replace') as infile:
                            # 写入分隔线和文件名
                            outfile.write(f"\n\n=== {file_path} ===\n\n")
                            # 写入文件内容
                            outfile.write(infile.read())
                    except Exception as e:
                        print(f"Error reading {file_path}: {str(e)}")

if __name__ == "__main__":
    target_directory = input("请输入要扫描的目录路径（默认为当前目录）: ") or '.'
    output_filename = input("请输入输出文件名（默认为combined_code.txt）: ") or 'combined_code.txt'
    
    combine_code_files(target_directory, output_filename)
    print(f"合并完成！输出文件：{output_filename}")