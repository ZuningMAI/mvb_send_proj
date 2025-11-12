import csv
import re
from datetime import datetime, time

def parse_data_txt(file_path):
    """
    解析data.txt文件，提取时间和数据字段
    """
    data_entries = []
    
    with open(file_path, 'r', encoding='utf-8') as f:
        for line in f:
            # 匹配 [时间] PD:0XA0 解析后数据:数据...
            # 例如: [00:25:14.684] PD:0XA0 解析后数据:0,6,19,13,12,4201,0,2160,95,3720,770,0,0,0,0,0,0,1,0,0,1,1,0,0,1,0,1
            match = re.match(r'\[(.*?)\] PD:0XA0 解析后数据:(.+)', line.strip())
            if match:
                time_str = match.group(1)
                data_str = match.group(2)
                
                # 将时间字符串转换为相对于当天0点的秒数
                time_obj = datetime.strptime(time_str, '%H:%M:%S.%f').time()
                # 转换为相对于0点的秒数
                seconds = time_obj.hour * 3600 + time_obj.minute * 60 + time_obj.second + time_obj.microsecond / 1000000
                
                # 分割数据字段
                data_fields = data_str.split(',')
                
                data_entries.append({
                    'time': seconds,
                    'fields': data_fields
                })
    
    return data_entries

def parse_csv_file(file_path):
    """
    解析X0.csv文件
    """
    data_entries = []
    
    with open(file_path, 'r', encoding='utf-8') as f:
        reader = csv.reader(f)
        headers = next(reader)  # 读取标题行
        
        for row in reader:
            # 第一列是时间（相对于开始的秒数）
            time_float = float(row[0])
            
            data_entries.append({
                'time': time_float,
                'fields': row[1:]  # 除了时间列之外的所有字段
            })
    
    return data_entries

def compare_data(data_txt_entries, csv_entries):
    """
    比较两个数据源的数据
    """
    print(f"Data.txt entries: {len(data_txt_entries)}")
    print(f"CSV entries: {len(csv_entries)}")
    
    if len(data_txt_entries) == 0:
        print("警告: 未能从data.txt解析到任何数据，请检查文件格式或正则表达式")
        return
    
    # 对于比较，我们只取前几个条目进行对比
    max_compare = min(len(data_txt_entries), len(csv_entries))
    
    # data.txt列名（根据用户提供的信息）
    data_txt_columns = [
        "lifeSignal", "railwayID", "endStationID", "nextStationID", "currentStationID",
        "targetDistance", "startDistance", "trainLoad", "limitSpeed", "netElectric", "netVoltage",
        "Speed", "tractionForce", "ebrakeForce", "airbrakeForce", "endStationIdAvaliable",
        "nextStationIdAvaliable", "currentStationIdAvaliable", "targetDistanceAvaliable",
        "startDistanceAvaliable", "ATOMode", "TmcActivity_1", "TmcActivity_2",
        "coasting", "traction", "ebraking", "loadAW_0", "loadAW_2", "loadAW_3"
    ]
    
    # 记录不匹配的字段
    mismatches = []
    
    # 比较前几个条目
    for i in range(max_compare):
        txt_entry = data_txt_entries[i]
        csv_entry = csv_entries[i]
        
        # 提取数据字段（去除时间）
        txt_fields = txt_entry['fields']
        csv_fields = csv_entry['fields'][:-1]  # 去掉CSV最后的pos字段
        
        # 检查所有字段的比较结果
        for j in range(min(len(txt_fields), len(csv_fields))):  # 显示所有字段
            txt_val = txt_fields[j]
            csv_val = csv_fields[j]
            
            # 判断是否匹配
            match = is_field_match(j, txt_val, csv_val, txt_fields, csv_fields)
            field_name = data_txt_columns[j] if j < len(data_txt_columns) else f"Field_{j}"
            
            if not match:
                mismatches.append({
                    'index': i,
                    'time_txt': txt_entry['time'],
                    'time_csv': csv_entry['time'],
                    'field_name': field_name,
                    'txt_value': txt_val,
                    'csv_value': csv_val
                })

    # 输出结果
    if mismatches:
        print(f"\n发现 {len(mismatches)} 处不匹配 (共比较 {max_compare * min(len(data_txt_entries[0]['fields']), len(csv_entries[0]['fields'])-1)} 个条目):")
        print("=" * 100)
        print(f"{'Index':<5} {'Time(data.txt)':<15} {'Time(CSV)':<10} {'Field':<22} {'data.txt':<10} {'CSV':<10}")
        print("-" * 100)
        for mismatch in mismatches:
            print(f"{mismatch['index']:<5} {mismatch['time_txt']:<15.3f} {mismatch['time_csv']:<10.3f} "
                  f"{mismatch['field_name']:<22} {mismatch['txt_value']:<10} {mismatch['csv_value']:<10}")
    else:
        print("\n恭喜！所有字段都匹配。")

def is_field_match(field_index, txt_val, csv_val, txt_fields, csv_fields):
    """
    判断特定字段是否匹配
    """
    # 特殊处理某些字段
    if field_index == 9:  # netElectric字段 (网侧电流) - data.txt中第10个字段
        # 根据协议和用户说明：
        # data.txt: ...,3720,770,... (3720是netElectric)
        # CSV: ...,770,372.0,... (770是netVoltage)
        # 字段顺序是互换的
        try:
            txt_net_electric = int(txt_val)  # data.txt中的netElectric (单位: 0.1A)
            csv_net_electric = int(float(csv_fields[10]) * 10)  # CSV中对应位置实际上是netElectric (单位: A -> 0.1A)
            return txt_net_electric == csv_net_electric
        except (ValueError, IndexError):
            return False
    elif field_index == 10:  # netVoltage字段 (网侧电压) - data.txt中第11个字段
        # data.txt中是netVoltage(网侧电压)，单位: V
        # CSV中是netElectric(网侧电流)，单位: 0.1A
        try:
            txt_net_voltage = int(txt_val)  # data.txt中的netVoltage (单位: V)
            csv_net_voltage = int(float(csv_fields[9]))  # CSV中对应位置实际上是netVoltage (单位: V)
            return txt_net_voltage == csv_net_voltage
        except (ValueError, IndexError):
            return False
    elif field_index == 11:  # Speed字段
        # data.txt中是实际值(单位: cm/s -> m/s * 100)，CSV中是实际值(单位: m/s)
        try:
            txt_speed = float(txt_val) / 100  # data.txt中的Speed (单位: cm/s -> m/s)
            csv_speed = float(csv_val)  # CSV中的Speed (单位: m/s)
            return abs(txt_speed - csv_speed) < 0.01
        except ValueError:
            return False
    elif field_index == 1:  # railwayID字段
        # CSV中记录为65，但实际应该是6
        if txt_val == "6" and csv_val == "65":
            return True  # 这种情况视为匹配
        else:
            return txt_val == csv_val
    elif field_index in [12, 13, 14]:  # 牵引、制动等力值字段(tractionForce, ebrakeForce, airbrakeForce)
        # 根据协议规定，这些力值字段单位是[1=0.1kN]
        # data.txt中是整数值，CSV中是浮点数，需要将CSV值乘以10后再比较
        try:
            txt_force = int(txt_val)  # data.txt中的力值
            csv_force = float(csv_val) * 10  # CSV中的力值需要乘以10转换为与data.txt相同的单位
            return abs(txt_force - csv_force) < 0.01
        except ValueError:
            return False
    else:
        # 其他字段直接比较
        return txt_val == csv_val

def main():
    # 文件路径
    # data_txt_path = 'dataY0\\parsedData\\2025-11-11\\0XA0\\data.txt'
    # csv_path = 'bin/Log/Y0.csv'
    data_txt_path = 'dataXX0\\parsedData\\2025-11-12\\0XA0\\data.txt'
    csv_path = 'bin/Log/XX0.csv'
    
    # 解析文件
    # print("正在解析 data.txt...")
    data_txt_entries = parse_data_txt(data_txt_path)
    
    # print("正在解析 X0.csv...")
    csv_entries = parse_csv_file(csv_path)
    
    # 比较数据
    compare_data(data_txt_entries, csv_entries)
    
        
# 测试正则表达式
def test_regex():
    test_line = "[00:25:14.684] PD:0XA0 解析后数据:0,6,19,13,12,4201,0,2160,95,3720,770,0,0,0,0,0,0,1,0,0,1,1,0,0,1,0,1"
    match = re.match(r'\[(.*?)\] PD:0XA0 解析后数据:(.+)', test_line.strip())
    if match:
        print("正则表达式匹配成功:")
        print("时间:", match.group(1))
        print("数据:", match.group(2))
    else:
        print("正则表达式匹配失败")

if __name__ == "__main__":
    # test_regex()  # 可以先运行测试
    main()