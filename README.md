# dmarc local client

### so far....
The dmarc_mail_app will fetch email from a server via port 110. It will parse and process the data that is typically found in a dmarc notifications from comcast.net, google.com, etc. At this point it will download all the emails of the account and parse the information into a set of text files to the folder 'test_files'.

### Requirments
**boost** is used extensivly.filesystem, spirit::x3, asio, archive...
You will need to run boost b2 and add at the least at this time, bzip2, as I wrote the zlib decompressor in the app, you don't nessaraly need that below.
```sh
b2.exe --with-iostreams -s BZIP2_SOURCE=C:\cpp\bzip2-1.0.6 -s ZLIB_SOURCE=C:\cpp\zlib-1.2.11
```
zlib.lib
Evetually, openssl.

### TODO
 - Add the SSL layer.
 - Use a database such as sqlite for storage.
 - A UI, probably wxWidgets, to operate the whole thing and present the data as tables.
 - Create a cmake file.

