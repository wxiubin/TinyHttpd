# TinyHttpd



## Usage

```
gcc httpd.c -o httpd

./httpd -h
// usage: httpd [-p <port>] [-d <http serve root directory>]
```

## TODO

- [x] Support custom port
- [x] Support GET method
- [ ] Support POST method
- [ ] The common MIME types
- [ ] Directory listings
- [ ] Support access log
- [ ] Use fork mode accept new conntion
- [ ] Support HTTP/1.1