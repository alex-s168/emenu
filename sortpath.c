#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/stat.h>

#include "slowdb/inc/slowdb.h"

typedef struct {
    uint32_t numuses;
    uint64_t high, low;
} sortkey;

static int sortkey_cmp(sortkey a, sortkey b)
{
    if (a.numuses < b.numuses)
        return 1;
    if (a.numuses > b.numuses)
        return -1;

    if (a.high < b.high)
        return -1;
    if (a.high > b.high)
        return  1;

    if (a.low < b.low)
        return -1;
    if (a.low > b.low)
        return  1;

    return 0;
}

static sortkey sortkey_calc(const char * str)
{
    uint8_t by[16] = { 
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0
    };

    size_t strl = strlen(str);
    if (strl > 16)
        strl = 16;
    memcpy(by, str, strl);

    sortkey res;
    res.numuses = 0;
    res.high = * (uint64_t*) &by[0];
    res.low = * (uint64_t*) &by[8];
    return res;
}

typedef struct node {
    char * path;
    struct node * next;
} node;

static void free_list(node* list)
{
    while (list != NULL)
    {
        free(list->path);
        node* next = list->next;
        free(list);
        list = next;
    }
}

static size_t lilen(node* n)
{
    size_t num = 0;
    while (n) {
        num ++;
        n = n->next;
    }
    return num;
}

static node* tail(node* n)
{
    while (n->next)
    {
        n = n->next;
    }
    return n;
}

static node* concat_list(node* a, node* b)
{
    if (a == NULL)
        return b;
    if (b == NULL)
        return a;

    tail(a)->next = b;
    return a;
}

static node* find_exec_in_dir(const char * dirp)
{
    DIR* d = opendir(dirp);
    if (d == NULL)
        return NULL;

    size_t dirplen = strlen(dirp);

    node * out = NULL;

    while (1) {
        struct dirent* ent = readdir(d);
        if (ent == NULL)
            break;

        if (!strcmp(ent->d_name, "."))
            continue;
        if (!strcmp(ent->d_name, ".."))
            continue;

        size_t entnamelen = strlen(ent->d_name);
        char * whole = malloc(dirplen + 1 + entnamelen + 1);
        if (!whole)
            continue;

        memcpy(whole, dirp, dirplen);
        whole[dirplen] = '/';
        memcpy(whole + dirplen + 1, ent->d_name, entnamelen + 1);

        // stat() resolves links
        struct stat s;
        if ( stat(whole, &s) != 0 )
        {
            free(whole);
            continue;
        }

        free(whole);

        if (! ( s.st_mode & S_IXUSR || s.st_mode & S_IXGRP ) )
            continue;

        char * fnam = strdup(ent->d_name);
        if ( fnam == NULL )
            continue;

        node *nd = malloc(sizeof(node));
        if (!nd)
        {
            free(fnam);
            continue;
        }

        nd->next = NULL;
        nd->path = fnam;

        // rev concat bc faster
        out = concat_list(nd, out);
    }

    closedir(d);

    return out;
}

static node* find_exec(void)
{
    const char * p;

    if ((p = getenv("PATH")) == NULL)
        return NULL;

    node* list = NULL;

    while (1) 
    {
        const char * next = strchr(p, ':');
        size_t len = next ? next - p : strlen(p);
        char * segment = strndup(p, len);
        if (!segment)
        {
            free_list(list);
            return NULL;
        }

        size_t segl = strlen(segment);
        if (segl == 0) 
        {
            free(segment);
            continue;
        }

        if (segment[segl-1] == '/')
        {
            segl --;
            segment[segl] = '\0';
        }

        node* app = find_exec_in_dir(segment);
        free(segment);

        // rev concat bc faster
        list = concat_list(app, list);

        if (next == NULL)
            break;
        p = next + 1;
    }

    return list;
}

typedef struct {
    sortkey sort;
    char * path;
} ent;

typedef struct {
    size_t num;
    ent * ents;
} entli;

static int sort__cmp(const void * ap, const void * bp)
{
    ent a = * (ent*) ap;
    ent b = * (ent*) bp;

    return sortkey_cmp(a.sort, b.sort);
}

static const char * dbpath = NULL;

static uint32_t getnuses(slowdb* db, const char * app)
{
    int vlen;
    uint32_t * uses_p = 
        (uint32_t *) slowdb_get(db, (uint8_t const*) app, strlen(app), &vlen);

    if (uses_p != NULL)
    {
        if (vlen != sizeof(uint32_t))
            perror("slowdb error");
        return *uses_p;
    }

    return 0;
}

static entli sort(node* l)
{
    entli li;
    li.num = lilen(l);
    li.ents = li.num ? malloc(li.num * sizeof(ent)) : NULL;
    if (li.ents == NULL)
    {
        li.num = 0;
        return li;
    }

    slowdb* db = slowdb_open(dbpath);
    if (db == NULL)
    {
        fprintf(stderr, "While trying to open db \"%s\"\n", dbpath);
        perror("slowdb error");
        exit(1);
    }

    size_t i = 0;
    while (l)
    {
        ent e;
        e.path = l->path;
        e.sort = sortkey_calc(l->path);
        e.sort.numuses = getnuses(db, l->path);

        li.ents[i++] = e;

        node* next = l->next;
        free(l);
        l = next;
    }

    slowdb_close(db);

    qsort(li.ents, li.num, sizeof(ent), sort__cmp);

    return li;
}

static void entli_free(entli li)
{
    for (size_t i = 0; i < li.num; i ++)
    {
        free(li.ents[i].path);
    }
    free(li.ents);
}

static void inv_usage(void)
{
    fprintf(stderr, "Invalid usage!\n");
    fprintf(stderr, "Usage: emenu_path <db path> [list|use <program>]\n");
}

int main(int argc, char ** argv)
{
    if (argc < 3) {
        inv_usage();
        return 1;
    }

    dbpath = argv[1];
    const char * mode = argv[2];

    if (!strcmp(mode, "list"))
    {
        if (argc != 3)
        {
            inv_usage();
            return 1;
        }

        entli path = sort(find_exec());
        if (path.ents == NULL)
            return 1;

        for (size_t i = 0; i < path.num; i ++)
        {
            char * p = path.ents[i].path;
            puts(p);
        }

        entli_free(path);
    }
    else if (!strcmp(mode, "use"))
    {
        if (argc != 4)
        {
            inv_usage();
            return 1;
        }

        const char * exe = argv[3];

        slowdb* db = slowdb_open(dbpath);
        if (db == NULL)
        {
            fprintf(stderr, "While trying to open db \"%s\"\n", dbpath);
            perror("slowdb error");
            exit(1);
        }

        uint32_t current = getnuses(db, exe);
        current ++;

        slowdb_replaceOrPut(db, 
                (uint8_t const *) exe, strlen(exe),
                (uint8_t const *) &current, sizeof(uint32_t));

        slowdb_close(db);
    }
    else
    {
        inv_usage();
        return 1;
    }

    return 0;
}
