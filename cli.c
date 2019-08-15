#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <error.h>
#include <errno.h>

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

#define FN "dl_out"

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int dl(const char *fn, char *range_header)
{
    CURL *curl;
    struct curl_slist *list = NULL;
    FILE *fp;
    int ret = 0;
    char path[128];
        struct stat stat_buf;

    sprintf(path, "http://localhost:8888/%s", fn);

    // curlのセットアップ
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, path);

    list = curl_slist_append(list, range_header);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    if (stat(FN, &stat_buf) == 0)
    {
        if (unlink(FN) != 0)
        {
            ret = 1;
            goto out;
        }
    }

    if ((fp = fopen(FN, "wb")) == NULL) {
        ret = 2;
        goto out;
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    // 実行
    if (curl_easy_perform(curl) != 0) {
        ret = 3;
        goto out;
    }

    ret = 0;

out:
    // 後始末
    curl_slist_free_all(list); /* free the list again */
    curl_easy_cleanup(curl);
    fclose(fp);

    return ret;
}

#define BUFSZ   (2*1024*1024)
int compare_out(const char *fdl, const char *forg, long start, long end)
{
    int ret = -1;
    int fd1, fd2;
    int sz1, sz2, sz, readsz;
    char buf1[BUFSZ], buf2[BUFSZ];

    if ((fd1 = open(fdl, O_RDONLY)) < 0) {
        ErrPrint("open %s failed: %s\n", fdl, strerror(errno));
        goto out;
    }

    if ((fd2 = open(forg, O_RDONLY)) < 0) {
        ErrPrint("open %s failed: %s\n", forg, strerror(errno));
        goto out;
    }

    if (lseek(fd2, start, SEEK_SET) < 0) {
        ErrPrint("lseek() failed:%s\n", strerror(errno));
        goto out;
    }

    sz = end - start + 1;
    while(1) {
        if (sz > BUFSZ) {
            readsz = BUFSZ;
        } else {
            readsz = sz;
        }
        if ((sz1 = read(fd1, buf1, readsz)) < 0) {
            ErrPrint("read() f1 failed:%s\n", strerror(errno));
            goto out;
        }
        if ((sz2 = read(fd2, buf2, readsz)) < 0) {
            ErrPrint("read() f2 failed:%s\n", strerror(errno));
            goto out;
        }

        if (sz1 != sz2) {
            DbgPrint("sz1:%d, sz2:%d\n", sz1, sz2);
            ret = 1;
            goto out;
        }

        if (sz1 == 0) {
            DbgPrint("data equal\n");
            break;
        }

        if (memcmp(buf1, buf2, sz1) != 0) {
            DbgPrint("data not equal\n");
            ret = 1;
            goto out;
        }

        sz -= sz1;
    }

    ret = 0;

out:
    close(fd1);
    close(fd2);
    return ret;
}

void usage(const char *cmd)
{
    printf("Usage: %s <file> <start> <end> \n", cmd);
}

int main(int argc, const char *argv[])
{
    long start, end;
    char rheader[128];
    int compare_res;

    if (argc != 4) {
        usage(argv[0]);
        return -1;
    }
    if ((start = atol(argv[2])) < 0) {
        fprintf(stderr, "start is minus\n");
        return -2;
    }
    if ((end = atol(argv[3])) <= 0) {
        fprintf(stderr, "end is too small\n");
        return -3;
    }
    sprintf(rheader, "Range: bytes=%l-%l", start, end);

    if (dl(argv[1], rheader) < 0) {
        ErrPrint("Download failed!\n");
        return -4;
    }

    compare_res = compare_out(FN, argv[1], start, end);
    if (compare_res < 0) {
        return -5;
    } else if (compare_res == 0) {
        return 0;
    } else {
        return 1;
    }

    return EXIT_SUCCESS;
}

