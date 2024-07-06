#!/bin/sh
set -e

# 使用userdel命令删除用户：sudo userdel username
# 该命令会将用户从系统中删除，但保留用户的主目录和个人文件。

# 使用userdel命令的-r选项：sudo userdel -r username
# 该命令会将用户从系统中删除，并且连同用户的主目录和个人文件一起删除。

# 添加用户 sudo useradd -m new_user
# 设置用户的默认shell为bash：
# sudo usermod -s /bin/bash new_user

# 如果出现is not in the sudoers file.  This incident will be reported
# vim /etc/sudoers
# your_username ALL=(ALL:ALL) ALL
# 如果是只读 sudo chmod +w /etc/sudoers
# sudo chmod -w /etc/sudoers

if [ "$(id -u)" -eq 0 ]; then
	echo "LOL"
else
	echo "permission denied "
fi
