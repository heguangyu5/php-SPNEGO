# 原始版本

http://pecl.php.net/package/krb5

# 相关资料

- https://en.wikipedia.org/wiki/Kerberos_(protocol)
- https://en.wikipedia.org/wiki/SPNEGO
- http://www.h5l.org/manual/HEAD/info/heimdal/What-is-Kerberos_003f.html
- http://www.h5l.org/manual/HEAD/info/heimdal/

# setup a KDC

    # server ip 192.168.3.106

    sudo apt-get install heimdal-kdc
    # 提示要填写的东西都空着就行

    $ sudo kadmin -l
    kadmin> init OURATS.COM
    kadmin> add hgy@OURATS.COM
    kadmin> add --random-key HTTP/test.spnego.com@OURATS.COM
    Max ticket life [1 day]:unlimited
    Max renewable life [1 week]:unlimited
    kadin> list -s *@OURATS.COM
    Principal                        Expiration  PW-exp  PW-change   Max life   Max renew
    hgy@OURATS.COM                   never       never   2016-08-11  1 day      1 week
    HTTP/test.spnego.com@OURATS.COM  never       never   2016-08-11  unlimited  unlimited
    kadmin> ext HTTP/test.spnego.com@OURATS.COM
    kadmin> exit

    $ sudo ktutil list
    FILE:/etc/krb5.keytab:

    Vno  Type                     Principal                        Aliases
      1  aes256-cts-hmac-sha1-96  HTTP/test.spnego.com@OURATS.COM
      1  des3-cbc-sha1            HTTP/test.spnego.com@OURATS.COM
      1  arcfour-hmac-md5         HTTP/test.spnego.com@OURATS.COM

    sudo cp /etc/krb5.keytab /tmp/HTTP-test.spnego.com.keytab
    sudo chmod +r HTTP-test.spnego.com.keytab

# setup a LAMP server

    # server ip 192.168.3.109

    $ cat test.spnego.com
    <VirtualHost *:80>
            ServerName test.spnego.com
            DocumentRoot /var/www/test.spnego.com
    </VirtualHost>

    sudo mkdir /var/www/test.spnego.com
    sudo cp test.php /var/www/test.spnego.com
    sudo a2ensite test.spnego.com

    # install this php extension
    sudo apt-get install php5-dev heimdal-dev make
    phpize
    ./configure
    make
    sudo make install
    sudo vi /etc/php5/conf.d/krb5.ini
    extension=krb5.so

    sudo service apache2 restart

    scp hgy@192.168.3.106:/tmp/HTTP-test.spnego.com.keytab .
    sudo vi /var/www/test.spnego.com/test.php
    /path/to/keytab => /home/hgy/HTTP-test.spnego.com.keytab

# 本机 (Ubuntu)

    sudo apt-get install heimdal-clients
    # Default Kerberos version 5 realm:
    OURATS.COM
    # Kerberos servers for your realm
    192.168.3.106
    # Admin server 空着就行


    $ kinit hgy
    $ klist
    Credentials cache: FILE:/tmp/krb5cc_1000
            Principal: hgy@OURATS.COM

      Issued                Expires               Principal
    Aug 11 15:35:04 2016  Aug 12 01:35:01 2016  krbtgt/OURATS.COM@OURATS.COM

    # add to /etc/hosts
    192.168.3.109 test.spnego.com

    # test with curl
    curl -v --negotiate -u : http://test.spnego.com/test.php

    # firefox
    about:config // search "nego"
    // set network.negotiate-auth.trusted-uris to test.spnego.com
    // 访问 http://test.spnego.com/test.php

    # chrome
    /opt/google/chrome/chrome --auth-server-whitelist="test.spnego.com"

# Windows 7

http://www.h5l.org/manual/HEAD/info/heimdal/Windows-compatibility.html

    # 192.168.3.106 kdc server
    sudo kadmin -l
    kadmin> add --password=123456 host/hgy-PC@OURATS.COM

    # windows
    // 以管理身份运行 cmd
    ksetup /setdomain OURATS.COM
    ksetup /addkdc OURATS.COM 192.168.3.106
    ksetup /SetComputerPassword 123456
    // 重启
    ksetup /mapuser hgy@OURATS.COM hgy
    // 注销,切换用户
    用户名: OURATS.COM/hgy
    密码: 123456
    // add hosts
    192.168.3.109 test.spnego.com
    // cmd
    klist

http://docs.alfresco.com/5.0/concepts/auth-kerberos-clientconfig.html
