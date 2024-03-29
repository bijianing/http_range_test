/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <microhttpd.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>

#define PORT 8888

#define __LOG_DEBUG             1
#define __LOG_ERROR             1

#if __LOG_DEBUG
#define DbgPrint(f, x...)      printf("DBG: %s():" f, __func__, ##x)
#else
#define DbgPrint(f, x...)
#endif

#if __LOG_ERROR
#define ErrPrint(f, x...)      fprintf(stderr, "ERR: %s():" f, __func__, ##x)
#else
#define ErrPrint(f, x...)
#endif

struct cls_struct
{
    int fd;
    long start;
    long end;
} data_cls;





long readdata(void *cls, unsigned long pos, char *buf, unsigned long max)
{
    struct cls_struct *dcls = cls;
    int fd = dcls->fd;
    int ret;

//    DbgPrint("pos:%08lx buf:%p size:%08lx\n", pos, buf, max);

    if (lseek(fd, dcls->start + pos, SEEK_SET) < 0) {
        fprintf(stderr, "lseek() failed:%s\n", strerror(errno));
        return -1;
    }

    if ((ret = read(fd, buf, max)) < 0) {
        fprintf(stderr, "read() failed:%s\n", strerror(errno));
        return -1;
    }

    return ret;
}

void freedata(void *cls)
{
    struct cls_struct *dcls = cls;
    int fd = dcls->fd;
    close(fd);
}

static int answer_to_connection(void *cls, struct MHD_Connection *connection,
                                const char *url, const char *method,
                                const char *version, const char *upload_data,
                                size_t *upload_data_size, void **con_cls)
{
    struct MHD_Response *response;
    int ret;

    DbgPrint("url:%s\n", url);
    if (strstr(url, ".mov"))
    {
        struct stat buffer;
        char path[128];
        const char *rheader;

        sprintf(path, ".%s", url);
        if (stat(path, &buffer) < 0)
        {
            fprintf(stderr, "stat(%s) failed:%s\n", path, strerror(errno));
            return MHD_NO;
        }

        rheader = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Range");
        if (rheader) {
            if (sscanf(rheader, "bytes=%ld-%ld", &data_cls.start, &data_cls.end) != 2) {
                data_cls.start = 0;
                data_cls.end = buffer.st_size;
            } else {

            }
        } else {
            data_cls.start = 0;
            data_cls.end = buffer.st_size;
        }

        if ((data_cls.fd = open(path, O_RDONLY)) < 0)
        {
            fprintf(stderr, "open(%s) failed:%s\n", path, strerror(errno));
            return MHD_NO;
        }

        response = MHD_create_response_from_callback(data_cls.end - data_cls.start, buffer.st_blksize, readdata, &data_cls, freedata);
    }
    else
    {
        const char *page = "<html><body>Hello, browser!</body></html>";
        response = MHD_create_response_from_buffer(strlen(page), (void *)page, MHD_RESPMEM_PERSISTENT);
    }

    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

int main()
{
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                              &answer_to_connection, NULL, MHD_OPTION_END);
    if (NULL == daemon)
        return 1;

    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}
