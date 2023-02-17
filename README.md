# SMTP and POP3 Server for Development and Testing
Testing emails during development and testing can be challenging. It is common to use tools like the [Fake SMTP Server](http://nilhcem.com/FakeSMTP/) for this. There are a few problems with these tool:

- They lack any POP3 or IMAP support. Which makes it harder for the testers to look at the emails remotely from their desktops. It will be much easier if a client like Outlook could fetch these emails.
- Some of them have GUI. This makes it harder to run the servers in background as a service.

Enter ``dev-mail``. This project provides an SMTP and POP3 server implementation. It requires no authentication and doesn't forward mails anywhere. This makes ``dev-mail`` well suited for development and testing. 

## Building
The software can be built in Linux or mac OS. 

>If you need to do a Linux build but do not have a Linux machine, then you can start a Docker container like this.
>
>```
>docker run --rm -v `pwd`:/usr/src -w /usr/src -it gcc
>```
>
>Then following the steps below.

First clone the required projects.

```
git clone https://github.com/bibhas2/Cute
git clone https://github.com/bibhas2/SockFramework
git clone https://github.com/bibhas2/dev-mail
```

Run ``make`` in these projects in order. For example.

```
cd Cute
make

cd ../SockFramework
make

cd ../dev-mail
make
```

## Running the Servers

The SMTP and POP3 server can be run as:

```
./dev-mail
```

By default, the server will listen on ports:

- 2525 for SMTP.
- 1100 for POP3.

Optionally, you can supply different port numbers.

```
./dev-mail --pop3-port 110 --smtp-port 25 
```

By default, the server logs verbose messages on standard output. You can disable that.

```
./dev-mail --quiet
```

### Setup a Systemd Service
In the ``/etc/systemd/system`` folder create a file called ``dev-mail.service``.

Add these lines. Change the path to the executable in ``ExecStart`` as needed.

```
[Unit]
Description=Developer Mail Server
[Service]
Type=simple
ExecStart=/path/to/dev-mail
Restart=on-failure
RestartSec=10
KillMode=process
[Install]
WantedBy=multi-user.target
```

Run these commands to enable the service.

```
sudo systemctl daemon-reload
sudo systemctl enable dev-mail
```

Finally, start the service.

```
sudo systemctl start dev-mail
```

Verify that the program is running.

```
sudo systemctl status dev-mail
```

You can debug any startup issues using this command.

```
sudo journalctl -f -a -u dev-mail
```

## More Details
### About Client Authentication
You can connect to it with or without authentication. If you authenticate then you can supply any user ID and password.

### About the SMTP Server
E-mail messages are saved in the ``mail/`` folder. You can pull these emails using a POP3 client like Outlook. Developers can also directly look at these mail files.

The SMTP server does not forward or relay the emails to anywhere. It is totally safe to send emails to any address during testing. These emails will never reach any actual users.

### About the POP3 Server
POP3 delivers any email message found in the ``mail/`` folder. This means, you can also manually export mail messages from Outlook and drop those files in the ``mail/`` folder. The POP3 server will deliver those messages.

The POP3 server supports the mail delete command. That means testers can remotely delete messages using a client like Outlook. You can also manually delete the files in the ``mail/`` folder.

