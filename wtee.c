/***********************************************************************************************************************************************************************
** @file:       wtee.c
** @author:     huixuan.li
** @date:       2024-05-24 14:53:39
** @brief:      windows下的tee命令实现，用于将程序标准输出同时打印在控制台和指定文件中，用法示例 example.exe | wtee log.txt
***********************************************************************************************************************************************************************/
#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#include <io.h>
#else
# error Your operating system is not supported！
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 1024

/* 返回码定义 */
enum{ SUCCESS, ARGS_ERROR, HANDLE_ERROR } EXITCODE;
/* 错误信息与帮助信息 */
static const char ARG_ERROR_INFO[] =
"ERROR! Couldn't recognize argument %s, please use option \"--help\" for instructions\n";
static const char HELP_INFO[] =
"The program can print the standard output to both the console and a specified file, \"-a\" option append the standard output to the specified file, as follows: \n"
"<command> | wtee [-a] <outputfile>\n";

/* 全局参数 */
static struct mode{
    bool if_append;
} mode = { false };
static char *file_path = NULL;
static char OUTPUT_FILE_BUFFER[BUFFER_SIZE];

/* 读取参数并根据参数修改全局参数, 若解析成功则返回true, 否则false */
static void analyze_arguments(int argc, char **argv);

/* 根据全局参数进行流处理 */
static void handle(void);

/* 主函数 */
int main(int argc, char *argv[]){
    analyze_arguments(argc, argv);
    handle();
    exit(EXITCODE = SUCCESS);
}


/* 读取参数并根据参数修改全局参数, 若解析成功则返回true, 否则false */
static void analyze_arguments(int argc, char **argv){
    // 对参数遍历处理
    for (int i = 1;i < argc;i++){
        char *p = argv[i];
        // 若以-开头,则判定为选项
        if (*p == '-'){
            // -a选项
            if (strcmp(p, "-a") == 0){
                mode.if_append = true;
            }
            // --help选项
            else if (strcmp(p, "--help") == 0){
                fputs(HELP_INFO, stdout);
                exit(EXITCODE = SUCCESS);
                // 未知选项
            } else{
                fprintf(stderr, ARG_ERROR_INFO, p);
                exit(EXITCODE = ARGS_ERROR);
            }
        }
        // 若参数不以-开头,则判定为输出文件名
        else{
            // 若存在多个文件名则报错
            if (file_path != NULL){
                fprintf(stderr, "Error! Multiple input_file set: %s and %s\n", file_path, p);
                exit(EXITCODE = ARGS_ERROR);
            }
            file_path = p;
        }

    }
    // 若最终无文件名，则展示帮助信息
    if (file_path == NULL){
        fputs(HELP_INFO, stdout);
        exit(EXITCODE = SUCCESS);
    }
}

/* 根据全局参数进行流处理 */
static void handle(void){
    FILE *fout;

    // 设置标准输入输出为二进制模式
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    // 打开输出文件流
    if (mode.if_append){
        fout = fopen(file_path, "ab");
    } else{
        fout = fopen(file_path, "wb");
    }
    if (fout == NULL){
        fprintf(stderr, "Error occur in open output_file %s, please check whether the path exists!\n", file_path);
        exit(EXITCODE = HANDLE_ERROR);
    }
    // 为输出文件设置缓冲区
    if (setvbuf(fout, OUTPUT_FILE_BUFFER, _IOFBF, BUFFER_SIZE)){
        fprintf(stderr, "ERROR! Couldn't set buffer for file %s", file_path);
        fclose(stdin);
        fclose(fout);
        exit(EXITCODE = HANDLE_ERROR);
    }

    // 读取标准输出的内容，输出至指定文件和标准输出
    char c;
    while (true){
        // 读入
        c = fgetc(stdin);
        // 若出现错误
        if (ferror(stdin)){
            fclose(stdin);
            fclose(fout);
            fprintf(stderr, "Unknown error occur in reading!\n");
            exit(EXITCODE = HANDLE_ERROR);
        }
        // 若超出末尾
        if (feof(stdin)){
            fclose(stdin);
            fclose(fout);
            return;
        }

        // 写出
        fputc(c, fout);
        fputc(c, stdout);
        if (ferror(fout)){
            fclose(stdin);
            fclose(fout);
            fprintf(stderr, "Unknown error occur in writing!\n");
            exit(EXITCODE = HANDLE_ERROR);
        }
        if (ferror(stdout)){
            fclose(stdin);
            fclose(fout);
            fprintf(stderr, "Unknown error occur in writing!\n");
            exit(EXITCODE = HANDLE_ERROR);
        }
    }
}
