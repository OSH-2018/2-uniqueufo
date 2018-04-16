# 操作系统实验（二）
### ——编写迷你shell程序
> **实验平台**： 
> Ubuntu 16.04(LTS)

### 实现功能
- 基本要求所要实现的全部功能：
    - cd /
    - pwd
    - ls
    - ls | wc   &emsp; **管道及多重管道**
    - ls | cat | wc
   - env
  - export MY_OWN_VAR=1
   - env | grep MY_OWN_VAR

- 支持文件重定向
  - 支持诸如 ls  >  out.txt 、ls  >>  out.txt 、cat  >  out.txt  <   in.txt这样的重定向指令('>',">>","<"符号两端需有空格)

- 支持后台处理（&）
