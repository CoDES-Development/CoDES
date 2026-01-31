total=0
max_mem=0

for i in {1..10}; do
    # 记录开始时间
    start=$(date +%s.%N)
    
    # 捕获 /usr/bin/time -l 输出
    time_output=$( { /usr/bin/time -l bash ./run-demo-codes-parallel-simulation.sh; } 2>&1 )
    # time_output=$( { /usr/bin/time -l bash ./run-demo-parallel-simulation.sh; } 2>&1 )
    
    end=$(date +%s.%N)
    
    # 计算耗时
    elapsed=$(echo "$end - $start" | bc)
    total=$(echo "$total + $elapsed" | bc)
    
    # 精准抓 maximum resident set size
    mem=$(echo "$time_output" | awk '/maximum resident set size/ {print $1}')
    
    # 防止空值
    if [ -z "$mem" ]; then
        mem=0
    fi
    
    # 更新最大内存
    if [ "$mem" -gt "$max_mem" ]; then
        max_mem=$mem
    fi
    
    # 格式化输出
    elapsed_fmt=$(printf "%.3f" "$elapsed")
    mem_mb=$(echo "scale=2; $mem/1024" | bc)
    echo "Run $i: $elapsed_fmt s, memory: ${mem_mb} MB"
done

avg=$(echo "$total / 10" | bc -l)
avg_fmt=$(printf "%.3f" "$avg")
max_mem_mb=$(echo "scale=2; $max_mem/1024" | bc)
echo "Average time: $avg_fmt s"
echo "Max memory used: ${max_mem_mb} MB"
