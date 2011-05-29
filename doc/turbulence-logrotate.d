/var/log/turbulence/*.log {
        daily
        missingok
        rotate 20
        compress
        delaycompress
        notifempty
        create 600 root root
        sharedscripts
        dateext
        postrotate
                /etc/init.d/turbulence reload > /dev/null
        endscript
}
