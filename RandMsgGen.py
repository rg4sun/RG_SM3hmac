import random

# 所选用的Unicode中的一些字符集, 数值为Unicode编码范围（16进制）
# 基本拉丁字母集           0020-007f
# 国际音标扩展集           0250-02af
# 希腊字母及科普特字母集    0370-03ff
# 常用标点集               2000-206f
# 货币符号集               20a0-20cf
# 数字形式集               2150-218f
# 数学运算符集             2200-22ff
# 杂项工业符号集           2300-23ff
# 带圈或括号的字母数字集    2460-24ff
# 制表符集                 2500-257f
# 印刷符号集               2700-27bf
# 杂项数学符号集            27c0-27ef
# 中日韩相关字符集          31c0-9fff

randMsg = ''
chrSet = [(0x0020,0x007f),
    (0x0250,0x02af),(0x0370,0x03ff),(0x2000,0x206f),(0x20a0,0x20cf),
    (0x2150,0x218f),(0x2200,0x22ff),(0x2300,0x23ff),(0x2460,0x24ff),
    (0x2500,0x257f),(0x2700,0x27bf),(0x27c0,0x27ef),(0x31c0,0x9fff)
    ]
chrAmount = 2**19 # 调节字符个数
for i in range(chrAmount):
    randMsg+=chr(random.choice([ random.randint(r[0],r[1]) for r in chrSet]))
# print(randMsg)
print("Generated randMsg length: %d"%(len(randMsg)))
f = open(".//TestSample//msg1M.txt","w+",encoding="UTF-8")
f.write(randMsg)
f.close()