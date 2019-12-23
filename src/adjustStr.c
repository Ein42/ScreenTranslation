#include "common.h"
#include "newWindow.h"
#include <ctype.h>
#include "dataStatistics.h"

int theThirdByteOfChinese ( char *c ) {

    /*此处需要了解一下utf8编码格式
     *
     * 对于只有一个字节的字符,单字节最高位为0，
     * 否则最高位开始往后数，有多少个1，就代表
     * 当前字符有多少字节，utf-8在本机测试为3个字节，
     * utf-8格式为111xxx 10xxxx 10xxxx(二进制表示).
     * 所以只要判断到当前字节的高两位为10,即十进制为2，且下一个字节
     * 的高两位不是10，即不是十进制中的2，说明单个汉字已经读取结束
     * 这个时候才可以对该字符后进行截取(加回车符),不然可能导致乱码
     *
     * 另说明这里与上0xff纯粹是为了有时打印成int型值时提供方便,因为int型
     * 有4个字节，需要截断才能显示正确的值*/
    return \
        (((*c & 0xff) >> 6) & 0x03) == 2  \
        && (((*(c+1) & 0xff) >> 6) & 0x03) != 2;
}

/*字符串调整函数:
 * 向每超过一定长度的字符串中添加回车字符,避免单行过长
 *
 * p[]指针数组，指向若干个待处理字符串
 * len 为单行所需截取长度
 * storage[] 指针数组，用来存储处理后的字符串
 */
void adjustStrForGoogle(char *p[], int len, char *storage[]) {

    int nowlen = 0;
    int asciich = 0;
    int cansplit = 0;

    for ( int i=0; i<3; i++ ) 
    {
        nowlen = 0;

        for ( int j=0, k=0; p[i] != NULL ; j++, k++ ) 
        {
            storage[i][k] = p[i][j];

            /*读到结尾字符时退出内层for循环，处理下一个字符串*/
            if ( p[i][j] == '\0' ) {
                if ( j != 0 && i != 0) {
                    strcat ( storage[i], "\n" );
                }
                break;
            }

            if ( p[i][j] == ' ')
                cansplit = k;

            if (theThirdByteOfChinese(&p[i][j])) {
                nowlen++;
                if ( k > cansplit )
                    cansplit = k;
            }

            if ( (p[i][j]&0xff) < 128 && (p[i][j]&&0xff >= 0) )
                asciich++;

            if ( asciich == 2 ) {
                nowlen++;
                asciich = 0;
            }

            /* 防止单词被割裂为两行(上一行末尾 下一行开头) */ 
            if (nowlen == len && isalnum(p[i][j]) && isalnum(p[i][j+1]))
            {
                int count = k - cansplit;

                /* 排除可分割下标在行头或者待回退字符过长,此种情况强制切割*/
                if (! (count > len || k == 0)) {

                    int goback = 0;
                    while (isalnum(p[i][j--]))
                    {
                        storage[i][k - goback++] = ' ';
                    }
                }
            }

            if ( nowlen == len ) {

                storage[i][++k] = '\n';
                nowlen = 0;
                continue;
            }
        }
    }
}

void adjustStrForBaidu(int len, char *source, int addSpace, int copy) {

    int nowlen = 0;
    int asciich = 0;
    int prefixLen = 0;
    int start = 0;
    int cansplit = 0;
    int notlock = 1;

    char buf[1024] = { '\0' };

    long int srclen = 0;
    srclen = strlen(source);

    if ( srclen > 1024 * 1024 ) {

        pbred("源数据过长，临时数组无法容纳");
        return;
    }

    char storage[1024*1024] = { '\0' };

    for ( int j=0, k=0; True ; j++, k++ ) 
    {
        storage[k] = source[j];

        if ( source[j] == '\0' )
            break;

        /* 空格为可分割字符，记录下该下标*/
        if ( source[j] == ' ')
            cansplit = k;

        if ( addSpace && source[j] == '.' && notlock ) {

            prefixLen = (j+1-start)*2;
            nowlen = (j-start)/2;
            notlock = 0;
        }

        /* 单个汉字的最后一个字节*/
        if (theThirdByteOfChinese(&source[j])) {
            nowlen++;

            /* 汉字可分割，如果下标大于上一个可分割空格字符的，则替换之*/
            if ( k > cansplit )
                cansplit = k;
        }

        /* ascii字符范围*/
        if ( isalpha(source[j]) )
            asciich++;

        /* 两个ascii码作一个字符长度*/
        if ( asciich == 2 ) {
            nowlen++;
            asciich = 0;
        }

        if ( nowlen == len ) {

            char nextchar = source[j+1];
            int forward = 0;
            int start = 0;
            int end = 0;

            /* 如果不是数字和字母，直接用回车符切割*/
            if ( ! isalnum(nextchar) ) {
                storage[++k] = '\n';
                nowlen = 0;
            } 
            else {

                /* 离上一个可分割字符的距离*/
                int count = k - cansplit;
                int right = k+1;
                int left = k;

                /* 如果上一个可分割字符在起点或者离上一个可分割字符超过一行的长度，那么强制分割，并返回for循环*/
                if ( count > len || cansplit == 0) {
                    storage[++k] = '\n';
                    nowlen = 0;
                    continue;
                }

                /*分割点前移了，再加回车符使原本已复制好的数据后移，先备份*/
                strncpy ( buf, &storage[cansplit+1], count );
                strcat ( buf, "\0" );

                /*后移的数据作为新行，同时其长度为当前新行长度*/
                nowlen = countCharNums ( buf );

                if ( addSpace ) {
                    right = right + prefixLen;
                    end = right; 
                }

                /*分割点后面原先数据整体后移一个字节*/
                while ( count-- ) {
                    storage[right] = storage[left];
                    right--;
                    left--;
                }

                storage[left+1] = '\n';

                k++;
                start = left + 2;
                forward = 1;
            }

            if ( addSpace ) {

                if ( forward == 1 )
                    k = start-1;

                int tmp = prefixLen;
                while(tmp--) {
                    storage[++k] = ' ';
                    nowlen++;
                }
                //nowlen /= 2;
                nowlen -= prefixLen/2;

                if ( forward == 1 )
                    k = end;
            }
            continue;
        }
    }

    if ( copy )
        strcpy(source, storage);
}
