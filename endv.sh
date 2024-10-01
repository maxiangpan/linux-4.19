#-gcc-aarch64-linux-gnu
#lib32stdc++6

sudo apt-get install \
qemu qemu-kvm libvirt-daemon \
libvirt-clients bridge-utils virt-manager \
repo git-core gitk git-gui gcc-arm-linux-gnueabihf \
u-boot-tools device-tree-compiler \
mtools parted \
bison flex libncurses-dev \
libudev-dev libusb-1.0-0-dev \
gcc-arm-linux-gnueabihf \
libssl-dev liblz4-tool genext2fs \
libsigsegv2 m4 intltool libdrm-dev \
curl sed make \
python3-venv python3-pip \
binutils build-essential gcc g++ bash patch gzip bzip2 perl tar cpio unzip rsync file bc wget \
libncurses5 libglib2.0-dev libgtk2.0-dev libglade2-dev cvs git mercurial rsync openssh-client \
subversion asciidoc w3m dblatex graphviz  libssl-dev texinfo fakeroot \
libbz2-dev libncurses5-dev libgdbm-dev liblzma-dev sqlite3 libsqlite3-dev \
openssl tcl8.6-dev tk8.6-dev libreadline-dev zlib1g-dev \
libparse-yapp-perl default-jre patchutils swig chrpath diffstat gawk time expect-dev -y

pip install Sphinx
sudo apt-get install python3-sphinx
pip3 install sphinx_rtd_theme==1.1.1

cd qemu
#不可使用sudo 如果是这样会引起一部分的报错
./configure
