#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parts.h"

#define MAX_LINE 256

static void usage(const char *prog) {
    printf("Usage: %s <command> [options]\n\n", prog);
    printf("Commands:\n");
    printf("  dump <ptable.bin> [outfile]   Convert a binary table to text\n");
    printf("  build <ptable.txt> [outfile]  Convert a text table to binary\n");
}

static char *strip(char *s) {
    char *end;

    while (*s && isspace((unsigned char)*s)) {
        s++;
    }

    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }

    return s;
}

static void trim_trailing_zero(char *dst, const uint8_t *src, size_t len) {
    size_t i;
    for (i = 0; i < len; ++i) {
        dst[i] = (char)src[i];
    }
    dst[len] = '\0';
    for (i = len; i > 0; --i) {
        if (dst[i - 1] == '\0' || dst[i - 1] == ' ') {
            dst[i - 1] = '\0';
        } else {
            break;
        }
    }
}

static void write_text(FILE *out, const struct ptable_t *ptable) {
    char buffer[17];
    int idx;

    trim_trailing_zero(buffer, ptable->version, 16);
    fprintf(out, "version=%s\n", buffer);

    trim_trailing_zero(buffer, ptable->product, 16);
    fprintf(out, "product=%s\n", buffer);

    fprintf(out, "tail=");
    for (idx = 0; idx < 32; ++idx) {
        fprintf(out, "%02x", ptable->tail[idx]);
    }
    fprintf(out, "\n\n");

    for (idx = 0; idx < 41; ++idx) {
        const struct ptable_line *line = &ptable->part[idx];
        if (line->name[0] == '\0') {
            break;
        }
        fprintf(out, "[partition]\n");
        fprintf(out, "name=%s\n", line->name);
        fprintf(out, "start=0x%x\n", line->start);
        fprintf(out, "length=0x%x\n", line->length);
        fprintf(out, "lsize=0x%x\n", line->lsize);
        fprintf(out, "loadaddr=0x%x\n", line->loadaddr);
        fprintf(out, "entry=0x%x\n", line->entry);
        fprintf(out, "nproperty=0x%x\n", line->nproperty);
        fprintf(out, "type=0x%x\n", line->type);
        fprintf(out, "count=0x%x\n\n", line->count);
        if (strcmp(line->name, "T") == 0) {
            break;
        }
    }
}

static int parse_tail(uint8_t *tail, const char *hex) {
    size_t len = strlen(hex);
    size_t i;

    if (len != 64) {
        return -1;
    }

    for (i = 0; i < 32; ++i) {
        char byte_str[3];
        unsigned value;
        byte_str[0] = hex[2 * i];
        byte_str[1] = hex[2 * i + 1];
        byte_str[2] = '\0';
        if (sscanf(byte_str, "%02x", &value) != 1) {
            return -1;
        }
        tail[i] = (uint8_t)value;
    }
    return 0;
}

static void clear_ptable(struct ptable_t *ptable) {
    memset(ptable, 0, sizeof(*ptable));
    memcpy(ptable->head, headmagic, sizeof(ptable->head));
}

static int parse_text(FILE *in, struct ptable_t *ptable) {
    char linebuf[MAX_LINE];
    int current = -1;

    clear_ptable(ptable);

    while (fgets(linebuf, sizeof(linebuf), in)) {
        char *line = strip(linebuf);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        if (strcmp(line, "[partition]") == 0) {
            if (current >= 40) {
                fprintf(stderr, "Too many partitions in text file\n");
                return -1;
            }
            current++;
            memset(&ptable->part[current], 0, sizeof(struct ptable_line));
            continue;
        }

        char *eq = strchr(line, '=');
        if (!eq) {
            fprintf(stderr, "Invalid line: %s\n", line);
            return -1;
        }
        *eq = '\0';
        char *key = strip(line);
        char *value = strip(eq + 1);

        if (strcmp(key, "version") == 0) {
            memset(ptable->version, 0, sizeof(ptable->version));
            strncpy((char *)ptable->version, value, sizeof(ptable->version));
        } else if (strcmp(key, "product") == 0) {
            memset(ptable->product, 0, sizeof(ptable->product));
            strncpy((char *)ptable->product, value, sizeof(ptable->product));
        } else if (strcmp(key, "tail") == 0) {
            if (parse_tail(ptable->tail, value) != 0) {
                fprintf(stderr, "Invalid tail value\n");
                return -1;
            }
        } else {
            if (current < 0) {
                fprintf(stderr, "Partition data before header\n");
                return -1;
            }
            struct ptable_line *lineptr = &ptable->part[current];
            if (strcmp(key, "name") == 0) {
                if (strlen(value) >= sizeof(lineptr->name)) {
                    fprintf(stderr, "Partition name too long: %s\n", value);
                    return -1;
                }
                memset(lineptr->name, 0, sizeof(lineptr->name));
                strcpy(lineptr->name, value);
            } else if (strcmp(key, "start") == 0) {
                lineptr->start = strtoul(value, NULL, 0);
            } else if (strcmp(key, "length") == 0) {
                lineptr->length = strtoul(value, NULL, 0);
            } else if (strcmp(key, "lsize") == 0) {
                lineptr->lsize = strtoul(value, NULL, 0);
            } else if (strcmp(key, "loadaddr") == 0) {
                lineptr->loadaddr = strtoul(value, NULL, 0);
            } else if (strcmp(key, "entry") == 0) {
                lineptr->entry = strtoul(value, NULL, 0);
            } else if (strcmp(key, "nproperty") == 0) {
                lineptr->nproperty = strtoul(value, NULL, 0);
            } else if (strcmp(key, "type") == 0) {
                lineptr->type = strtoul(value, NULL, 0);
            } else if (strcmp(key, "count") == 0) {
                lineptr->count = strtoul(value, NULL, 0);
            } else {
                fprintf(stderr, "Unknown key: %s\n", key);
                return -1;
            }
        }
    }

    if (current >= 0 && current < 40) {
        ptable->part[current + 1].name[0] = '\0';
    }

    return 0;
}

static int dump_bin(const char *inpath, const char *outpath) {
    FILE *in = fopen(inpath, "rb");
    if (!in) {
        perror("fopen");
        return -1;
    }
    struct ptable_t ptable;
    if (fread(&ptable, sizeof(ptable), 1, in) != 1) {
        fclose(in);
        fprintf(stderr, "Unable to read partition table\n");
        return -1;
    }
    fclose(in);

    FILE *out = stdout;
    if (outpath) {
        out = fopen(outpath, "w");
        if (!out) {
            perror("fopen");
            return -1;
        }
    }

    if (memcmp(ptable.head, headmagic, sizeof(headmagic)) != 0) {
        fprintf(stderr, "Warning: head magic does not match\n");
    }

    write_text(out, &ptable);

    if (outpath) {
        fclose(out);
    }

    return 0;
}

static int build_bin(const char *inpath, const char *outpath) {
    FILE *in = fopen(inpath, "r");
    if (!in) {
        perror("fopen");
        return -1;
    }
    struct ptable_t ptable;
    if (parse_text(in, &ptable) != 0) {
        fclose(in);
        return -1;
    }
    fclose(in);

    FILE *out = stdout;
    if (outpath) {
        out = fopen(outpath, "wb");
        if (!out) {
            perror("fopen");
            return -1;
        }
    }

    if (fwrite(&ptable, sizeof(ptable), 1, out) != 1) {
        perror("fwrite");
        if (outpath) {
            fclose(out);
        }
        return -1;
    }

    if (outpath) {
        fclose(out);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "dump") == 0) {
        const char *outfile = (argc > 3) ? argv[3] : NULL;
        return dump_bin(argv[2], outfile);
    } else if (strcmp(argv[1], "build") == 0) {
        const char *outfile = (argc > 3) ? argv[3] : NULL;
        return build_bin(argv[2], outfile);
    } else {
        usage(argv[0]);
        return 1;
    }
}
