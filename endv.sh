#!/bin/bash

function default(){
    echo "***********************"
    echo "*1.qemu_env           *"
    echo "*2.opengrok_env       *"  
    echo "***********************"
}

function qemu_env(){
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

    sudo apt-get install gcc-arm-linux-gnueabi #编译arm工具链
    sudo apt-get install gcc-aarch64-linux-gnu #编译arm64工具链 
    pip install Sphinx
    sudo apt-get install python3-sphinx
    sudo apt-get install ninja-build
    pip3 install sphinx_rtd_theme==1.1.1

    cd qemu
    git clone https://gitlab.freedesktop.org/slirp/libslirp.git
    cd libslirp
    sudo apt install meson
    meson build  
    ninja -C build install
    #不可使用sudo 如果是这样会引起一部分的报错
    ./configure
    make && sudo make install
}

text='
[Unit]
Description=Tomcat
After=syslog.target network.target
[Service]
Type=forking
User=tomcat
Group=tomcat
Environment=JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
Environment='JAVA_OPTS=-Djava.awt.headless=true'
Environment=CATALINA_HOME=/usr/share/apache-tomcat
Environment=CATALINA_BASE=/usr/share/apache-tomcat
Environment=CATALINA_PID=/usr/share/apache-tomcat/temp/tomcat.pid
ExecStart=/usr/share/apache-tomcat/bin/catalina.sh start
ExecStop=/usr/share/apache-tomcat/bin/catalina.sh stop
[Install]
WantedBy=multi-user.target
'

text_opengrok='
<?xml version="1.0" encoding="UTF-8"?>
<web-app xmlns="https://jakarta.ee/xml/ns/jakartaee"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="https://jakarta.ee/xml/ns/jakartaee
         https://jakarta.ee/xml/ns/jakartaee/web-app_6_0.xsd"
         version="6.0">

    <display-name>OpenGrok</display-name>
    <description>A wicked fast source browser</description>
    <context-param>
        <description>Full path to the configuration file where OpenGrok can read its configuration</description>
        <param-name>CONFIGURATION</param-name>
        <param-value>/opengrok/etc/configuration.xml</param-value>
    </context-param>
    <listener>
        <listener-class>org.opengrok.web.WebappListener</listener-class>
    </listener>
    <filter>
        <filter-name>StatisticsFilter</filter-name>
        <filter-class>org.opengrok.web.StatisticsFilter</filter-class>
    </filter>
    <filter-mapping>
        <filter-name>StatisticsFilter</filter-name>
        <url-pattern>/*</url-pattern>
    </filter-mapping>
    <filter>
        <filter-name>AuthorizationFilter</filter-name>
        <filter-class>org.opengrok.web.AuthorizationFilter</filter-class>
    </filter>
    <filter-mapping>
        <filter-name>AuthorizationFilter</filter-name>
        <url-pattern>/*</url-pattern>
    </filter-mapping>
    <filter>
        <filter-name>ExpiresHalfHourFilter</filter-name>
        <filter-class>org.opengrok.web.ResponseHeaderFilter</filter-class>
        <init-param>
            <param-name>Cache-Control</param-name>
            <param-value>max-age=1800</param-value>
        </init-param>
    </filter>
    <filter>
        <filter-name>ExpiresOneDayFilter</filter-name>
        <filter-class>org.opengrok.web.ResponseHeaderFilter</filter-class>
        <init-param>
            <param-name>Cache-Control</param-name>
            <param-value>max-age=86400</param-value>
        </init-param>
    </filter>
    <filter>
        <filter-name>ExpiresOneYearFilter</filter-name>
        <filter-class>org.opengrok.web.ResponseHeaderFilter</filter-class>
        <init-param>
            <param-name>Cache-Control</param-name>
            <param-value>max-age=31536000</param-value>
        </init-param>
    </filter>
    <filter-mapping>
        <filter-name>ExpiresOneDayFilter</filter-name>
        <url-pattern>/manifest.json</url-pattern>
        <dispatcher>REQUEST</dispatcher>
    </filter-mapping>
    <filter-mapping>
        <filter-name>ExpiresHalfHourFilter</filter-name>
        <url-pattern>/history/*</url-pattern>
        <url-pattern>/raw/*</url-pattern>
        <url-pattern>/download/*</url-pattern>
        <url-pattern>/rss/*</url-pattern>
        <url-pattern>/error</url-pattern>
        <url-pattern>/enoent</url-pattern>
        <url-pattern>/eforbidden</url-pattern>
        <dispatcher>REQUEST</dispatcher>
    </filter-mapping>
    <filter-mapping>
        <filter-name>ExpiresOneYearFilter</filter-name>
        <url-pattern>/default/*</url-pattern>
        <url-pattern>/js/*</url-pattern>
        <url-pattern>/webjars/*</url-pattern>
        <dispatcher>REQUEST</dispatcher>
    </filter-mapping>
    <filter>
        <filter-name>CookieFilter</filter-name>
        <filter-class>org.opengrok.web.CookieFilter</filter-class>
        <init-param>
            <param-name>SameSite</param-name>
            <param-value>Strict</param-value>
        </init-param>
        <init-param>
            <param-name>Secure</param-name>
            <param-value></param-value>
        </init-param>
    </filter>
    <filter-mapping>
        <filter-name>CookieFilter</filter-name>
        <url-pattern>/*</url-pattern>
        <dispatcher>REQUEST</dispatcher>
    </filter-mapping>
    <servlet>
        <display-name>Source Finder</display-name>
        <servlet-name>search</servlet-name>
        <jsp-file>/search.jsp</jsp-file>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>Source History</display-name>
        <servlet-name>history</servlet-name>
        <jsp-file>/history.jsp</jsp-file>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>Source lister</display-name>
        <servlet-name>lister</servlet-name>
        <jsp-file>/list.jsp</jsp-file>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>Source diffs between revisions</display-name>
        <servlet-name>diff</servlet-name>
        <jsp-file>/diff.jsp</jsp-file>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>Shows more matching lines</display-name>
        <servlet-name>more</servlet-name>
        <jsp-file>/more.jsp</jsp-file>
    </servlet>
    <servlet>
        <display-name>Source Changes in RSS format</display-name>
        <servlet-name>rss</servlet-name>
        <jsp-file>/rss.jsp</jsp-file>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>OpenSearch link for current project</display-name>
        <servlet-name>opensearch</servlet-name>
        <jsp-file>/opensearch.jsp</jsp-file>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>Raw Source lister</display-name>
        <servlet-name>raw</servlet-name>
        <servlet-class>org.opengrok.web.GetFile</servlet-class>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>Download source</display-name>
        <servlet-name>download</servlet-name>
        <servlet-class>org.opengrok.web.GetFile</servlet-class>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>Error Handler</display-name>
        <servlet-name>error</servlet-name>
        <jsp-file>/error.jsp</jsp-file>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>File not found handler</display-name>
        <servlet-name>enoent</servlet-name>
        <jsp-file>/enoent.jsp</jsp-file>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet>
        <display-name>Forbidden error handler</display-name>
        <servlet-name>eforbidden</servlet-name>
        <jsp-file>/eforbidden.jsp</jsp-file>
        <init-param>
            <param-name>keepgenerated</param-name>
            <param-value>true</param-value>
        </init-param>
    </servlet>
    <servlet-mapping>
        <servlet-name>search</servlet-name>
        <url-pattern>/search</url-pattern>
        <!-- SEARCH_P -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>opensearch</servlet-name>
        <url-pattern>/opensearch</url-pattern>
        <!-- SEARCH_O -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>history</servlet-name>
        <url-pattern>/history/*</url-pattern>
        <!-- HIST_L -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>lister</servlet-name>
        <url-pattern>/xref/*</url-pattern>
        <!-- XREF_P -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>raw</servlet-name>
        <url-pattern>/raw/*</url-pattern>
        <!-- RAW_P -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>download</servlet-name>
        <url-pattern>/download/*</url-pattern>
        <!-- DOWNLOAD_P -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>search</servlet-name>
        <url-pattern>/s</url-pattern>
        <!-- SEARCH_R -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>diff</servlet-name>
        <url-pattern>/diff/*</url-pattern>
        <!-- DIFF_P -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>more</servlet-name>
        <url-pattern>/more/*</url-pattern>
        <!-- MORE_P -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>rss</servlet-name>
        <url-pattern>/rss/*</url-pattern>
        <!-- RSS_P -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>error</servlet-name>
        <url-pattern>/error</url-pattern>
        <!-- ERROR -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>enoent</servlet-name>
        <url-pattern>/enoent</url-pattern>
        <!-- NOT_FOUND -->
    </servlet-mapping>
    <servlet-mapping>
        <servlet-name>eforbidden</servlet-name>
        <url-pattern>/eforbidden</url-pattern>
        <!-- FORBIDDEN -->
    </servlet-mapping>
    <error-page>
        <error-code>404</error-code>
        <location>/enoent</location>
    </error-page>
    <error-page>
        <error-code>403</error-code>
        <location>/eforbidden</location>
    </error-page>
    <error-page>
        <error-code>500</error-code>
        <location>/error</location>
    </error-page>
    <jsp-config>
        <jsp-property-group>
            <url-pattern>*.jsp</url-pattern>
            <trim-directive-whitespaces>true</trim-directive-whitespaces>
        </jsp-property-group>
    </jsp-config>
</web-app>
'

function opengrok_env(){
    echo "https://blog.csdn.net/weixin_46557083/article/details/125895268?spm=1001.2101.3001.6650.2&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ECtr-2-125895268-blog-34984805.235%5Ev43%5Epc_blog_bottom_relevance_base6&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ECtr-2-125895268-blog-34984805.235%5Ev43%5Epc_blog_bottom_relevance_base6&utm_relevant_index=5"
    echo "https://www.cnblogs.com/idorax/p/13796481.html#:~:text=OpenGrok"
    echo "install java.."
    apt-get install openjdk-11-jdk

    echo "install tomcat.."
    wget https://archive.apache.org/dist/tomcat/tomcat-10/v10.0.14/bin/apache-tomcat-10.0.14.tar.gz
    tar -zxvf apache-tomcat-10.0.14.tar.gz -C /usr/share/
    cd /usr/share && sudo mv apache-tomcat-10.0.14 apache-tomcat
    useradd -M -d /usr/share/apache-tomcat tomcat
    chown -R tomcat /usr/share/apache-tomcat
    sudo echo "$text" >/etc/systemd/system/tomcat.service
    systemctl daemon-reload
    systemctl restart tomcat
    systemctl enable tomcat
    ss -tunelp | grep 8080

    echo "install ctags.."
    git clone git@github.com:universal-ctags/ctags.git
    cd ctags
    ./autogen.sh
    ./configure  # use --prefix=/where/you/want to override installation directory, defaults to /usr/local
    make
    sudo make install
    ctags --version

    echo "install opengrok.."
    mkdir -p /opengrok/{src,data,dist,etc,log}
    cd /opt
    sudo wget https://github.com/oracle/opengrok/releases/download/1.13.22/opengrok-1.13.22.tar.gz
    sudo tar -C /opengrok/dist --strip-components=1 -xzf /opt/opengrok-1.13.22.tar.gz
    sudo tar -C /opengrok/dist --strip-components=1 -xzf /opt/opengrok-1.13.22.tar.gz 
    sudo cp /opengrok/dist/doc/logging.properties /opengrok/etc
    cd /opengrok/dist/tools
    pip3 install opengrok-tools.tar.gz
    
    sudo cp /opengrok/dist/lib/source.war /usr/share/apache-tomcat/webapps/source.war
    sudo cp /usr/share/apache-tomcat/webapps/source/WEB-INF/web.xml /tmp/web.xml
    sudo cp /usr/share/apache-tomcat/webapps/source/WEB-INF/web.xml /usr/share/apache-tomcat/webapps/source/WEB-INF/web.xml.bak
    sudo rm /usr/share/apache-tomcat/webapps/source/WEB-INF/web.xml
    touch /usr/share/apache-tomcat/webapps/source/WEB-INF/web.xml
    echo "$text_opengrok" >> /usr/share/apache-tomcat/webapps/source/WEB-INF/web.xml
    cd /opengrok/src 
    sudo git clone https://github.com/githubtraining/hellogitworld.git 
    sudo java \
        -Djava.util.logging.config.file=/opengrok/etc/logging.properties \
        -jar /opengrok/dist/lib/opengrok.jar \
        -c /usr/local/bin/ctags \
        -s /opengrok/src \
        -d /opengrok/data -H -P -S -G \
        -W /opengrok/etc/configuration.xml \
        -U http://localhost:8080/source
    echo "code in /opengrok/src/"
}

function clangd_env(){
    sudo apt install bear
    bear --version
    sudo apt-get install clangd
    which clangd
    echo "  -compile-commands-dir=${workspaceFolder}
            -background-index
            -completion-style=detailed
            -header-insertion=never
            -log=info"
    echo "https://blog.csdn.net/ludaoyi88/article/details/135051470#:~:text=%E7%9C%8B%E4%BB%A3%E7%A0%81%E7%A5%9E%E5%99%A8%EF%BC%9Avs#:~:text=%E7%9C%8B%E4%BB%A3%E7%A0%81%E7%A5%9E%E5%99%A8%EF%BC%9Avs"
}

OPTIONS="${@:-default}"

default
read -p "Choose your current operation: " choice

for option in "${OPTIONS[@]}"; do
    case $choice in
        "qemu"|"1")
            qemu_env ;;
        "opengrok"|"2")
            opengrok_env ;;
        "clangd"|"3")
            clangd_env ;;
        *)
            echo "Invalid option" ;;
    esac
done