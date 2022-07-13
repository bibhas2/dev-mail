# SMTP and POP3 Server for Development and Testing
Testing emails during development and testing can be challenging. It is common to use tools like the [Fake SMTP Server](http://nilhcem.com/FakeSMTP/) for this. There are a few problems with these:

- They lack any POP3 or IMAP support. Which makes it harder for the testers to look at the emails remotely from their desktops. It will be much easier if a client like Outlook could fetch these emails.
- Some of them have GUI. This makes it harder to run the servers in background as a service.

Enter ``dev-mail``. This project provides an SMTP and POP3 server implementation. They are ideally suited for development and testing. 

## Building
The software can be built in Linux or mac OS. 

>If you need to do a Linux build but do not have a Linux machine, then you can start a Docker container like this.
>
>```
>docker run --rm -v `pwd`:/usr/src -w /usr/src -it gcc
>```

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

It will listen on ports:

- 2525 for SMTP.
- 1010 for POP3.

### About Client Authentication
You can connect to it with or without authentication. If you authenticate then you can supply any user ID and password.

### About the SMTP Server
E-mail messages are saved in the ``mail/`` folder. Developers can directly look at these mail files. During system testing by testers, it may be easier to pull these emails from Outlook or similar clients using POP3.

The SMTP server does not forward or relay the emails to anywhere. It is totally safe to send emails to any address during testing. These emails will never reach any actual users.

### About the POP3 Server
POP3 delivers any email message found in the ``mail/`` folder. This means, you can also manually export mail messages from Outlook and drop those files in the ``mail/`` folder. The POP3 server will deliver those messages.

The POP3 server supports the mail delete command. That means testers can remotely delete messages using a client like Outlook. You can also manually delete the files in the ``mail/`` folder.

