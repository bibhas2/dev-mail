# SMTP and POP3 Server for Development and Testing
Testing emails during development and testing can be challenging. It is common to use tools like the [Fake SMTP Server](http://nilhcem.com/FakeSMTP/) for this. There are a few problems with these:

- They lack any POP3 or IMAP support. Which makes it harder for the testers to look at the emails remotely from their desktops. It will be much easier if a client like Outlook could fetch these emails.
- Some of them have GUI. This makes it harder to run the servers in background as a service.

Enter ``dev-mail``. This project provides an SMTP and POP3 server implementation. They are ideally suited for development and testing. 

## Building
The software can be built in Linux or mac OS.

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

## Running the SMTP Server

The SMTP server can be run as:

```
./dev-smtp
```

It will listen on port 2525.

You can connect to it with or without authentication. If you authenticate then you can supply any user ID and password.

E-mail messages are saved in the ``mail/`` folder. Developers can directly look at these mail files. During system testing by testers, it may be easier to pull these emails from Outlook or similar clients using POP3.

The SMTP server does not forward or relay the emails to anywhere. It is totally safe to send emails to any address during testing. These emails will never reach any actual users.

## Running the POP3 Server

Run the POP3 server as follows.

```
./dev-pop3
```

It listens on port 1010.

You can connect to it using any user ID and password. 

It delivers any email message found in the ``mail/`` folder.

It supports the mail delete command. That means testers can remotely delete messages using a client like Outlook. You can also manually delete the files in the ``mail/`` folder.

