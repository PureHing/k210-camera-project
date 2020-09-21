#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "parsed_url.h"
#include "my_str.h"

#include "printf.h"

/*********************************************************/
/*
	Free memory of parsed url
 */
void parsed_url_free(struct parsed_url *purl)
{
    if (NULL != purl)
    {
        if (NULL != purl->scheme)
            free(purl->scheme);
        if (NULL != purl->host)
            free(purl->host);
        if (NULL != purl->port)
            free(purl->port);
        if (NULL != purl->path)
            free(purl->path);
        if (NULL != purl->query)
            free(purl->query);
        if (NULL != purl->ip)
            free(purl->ip);
            
#if NOT_ONLY_SUPPORT_GET
        if (NULL != purl->fragment)
            free(purl->fragment);
        if (NULL != purl->username)
            free(purl->username);
        if (NULL != purl->password)
            free(purl->password);
#endif

        free(purl);
    }
}

static int mytolower(int c)
{
    if ((c >= 'A') && (c <= 'Z'))
        return c + ('a' - 'A');
    return c;
}

//判断字符c是否为英文字母
/*
	Check whether the character is permitted in scheme string
 */
static int my_is_scheme_char(int c)
{
    return (!my_isalpha(c) && '+' != c && '-' != c && '.' != c) ? 0 : 1;
}

/*
	Parses a specified URL and returns the structure named 'parsed_url'
	Implented according to:
	RFC 1738 - http://www.ietf.org/rfc/rfc1738.txt
	RFC 3986 -  http://www.ietf.org/rfc/rfc3986.txt
 */
struct parsed_url *parse_url(const char *url)
{
    /* Define variable */
    struct parsed_url *purl;
    const char *tmpstr;
    const char *curstr;
    int len;
    int i;
    int userpass_flag;
    int bracket_flag;
    // unsigned int dsst;

    /* Allocate the parsed url storage */
    purl = (struct parsed_url *)malloc(sizeof(struct parsed_url));
    if (NULL == purl)
    {
        return NULL;
    }
    purl->scheme = NULL;
    purl->host = NULL;
    purl->port = NULL;
    purl->path = NULL;
    purl->query = NULL;
#if NOT_ONLY_SUPPORT_GET

    purl->fragment = NULL;
    purl->username = NULL;
    purl->password = NULL;
#endif

    curstr = url;

    /*
     * <scheme>:<scheme-specific-part>
     * <scheme> := [a-z\+\-\.]+
     *             upper case = lower case for resiliency
     */
    /* Read scheme */
    tmpstr = strchr(curstr, ':');
    if (NULL == tmpstr)
    {
        parsed_url_free(purl);
        printk("Error on line %d (%s)\n", __LINE__, __FILE__);
        return NULL;
    }

    /* Get the scheme length */
    len = tmpstr - curstr;

    /* Check restrictions */
    for (i = 0; i < len; i++)
    {
        if (my_is_scheme_char(curstr[i]) == 0)
        {
            /* Invalid format */
            parsed_url_free(purl);
            printk("Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
    }
    /* Copy the scheme to the storage */
    purl->scheme = (char *)malloc(sizeof(char) * (len + 1));
    if (NULL == purl->scheme)
    {
        parsed_url_free(purl);
        printk("Error on line %d (%s)\n", __LINE__, __FILE__);
        return NULL;
    }

    (void)strncpy(purl->scheme, curstr, len);
    purl->scheme[len] = '\0';

    /* Make the character to lower if it is upper case. */
    for (i = 0; i < len; i++)
    {
        purl->scheme[i] = mytolower(purl->scheme[i]);
    }

    /* Skip ':' */
    tmpstr++;
    curstr = tmpstr;

    /*
     * //<user>:<password>@<host>:<port>/<url-path>
     * Any ":", "@" and "/" must be encoded.
     */
    /* Eat "//" */
    for (i = 0; i < 2; i++)
    {
        if ('/' != *curstr)
        {
            parsed_url_free(purl);
            printk("Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        curstr++;
    }
#if NOT_ONLY_SUPPORT_GET
    /* Check if the user (and password) are specified. */
    userpass_flag = 0;
    tmpstr = curstr;
    while ('\0' != *tmpstr)
    {
        if ('@' == *tmpstr)
        {
            /* Username and password are specified */
            userpass_flag = 1;
            break;
        }
        else if ('/' == *tmpstr)
        {
            /* End of <host>:<port> specification */
            userpass_flag = 0;
            break;
        }
        tmpstr++;
    }

    /* User and password specification */
    tmpstr = curstr;
    if (userpass_flag)
    {
        /* Read username */
        while ('\0' != *tmpstr && ':' != *tmpstr && '@' != *tmpstr)
        {
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->username = (char *)malloc(sizeof(char) * (len + 1));
        if (NULL == purl->username)
        {
            parsed_url_free(purl);
            printk("Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        (void)strncpy(purl->username, curstr, len);
        purl->username[len] = '\0';

        /* Proceed current pointer */
        curstr = tmpstr;
        if (':' == *curstr)
        {
            /* Skip ':' */
            curstr++;

            /* Read password */
            tmpstr = curstr;
            while ('\0' != *tmpstr && '@' != *tmpstr)
            {
                tmpstr++;
            }
            len = tmpstr - curstr;
            purl->password = (char *)malloc(sizeof(char) * (len + 1));
            if (NULL == purl->password)
            {
                parsed_url_free(purl);
                printk("Error on line %d (%s)\n", __LINE__, __FILE__);
                return NULL;
            }
            (void)strncpy(purl->password, curstr, len);
            purl->password[len] = '\0';
            curstr = tmpstr;
        }
        /* Skip '@' */
        if ('@' != *curstr)
        {
            parsed_url_free(purl);
            printk("Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        curstr++;
    }
#endif
    if ('[' == *curstr)
    {
        bracket_flag = 1;
    }
    else
    {
        bracket_flag = 0;
    }
    /* Proceed on by delimiters with reading host */
    tmpstr = curstr;
    while ('\0' != *tmpstr)
    {
        if (bracket_flag && ']' == *tmpstr)
        {
            /* End of IPv6 address. */
            tmpstr++;
            break;
        }
        else if (!bracket_flag && (':' == *tmpstr || '/' == *tmpstr))
        {
            /* Port number is specified. */
            break;
        }
        tmpstr++;
    }
    len = tmpstr - curstr;
    purl->host = (char *)malloc(sizeof(char) * (len + 1));
    if (NULL == purl->host || len <= 0)
    {
        parsed_url_free(purl);
        printk("Error on line %d (%s)\n", __LINE__, __FILE__);
        return NULL;
    }
    (void)strncpy(purl->host, curstr, len);
    purl->host[len] = '\0';
    curstr = tmpstr;

    /* Is port number specified? */
    if (':' == *curstr)
    {
        curstr++;
        /* Read port number */
        tmpstr = curstr;
        while ('\0' != *tmpstr && '/' != *tmpstr)
        {
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->port = (char *)malloc(sizeof(char) * (len + 1));
        if (NULL == purl->port)
        {
            parsed_url_free(purl);
            printk("Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        (void)strncpy(purl->port, curstr, len);
        purl->port[len] = '\0';
        curstr = tmpstr;
    }
    else
    {
        purl->port = (char *)malloc(sizeof(char) * (4));
        if (NULL == purl->port)
        {
            parsed_url_free(purl);
            printk("Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        /* fixbug */
        sprintf(purl->port, "80");
        purl->port[2] = '\0';
        //		purl->port = "80";
    }

    /* Get ip */
    //	ecos_hostname_to_ip(purl->host,ip);
    //  purl->ip = ip;

    purl->ip = (char *)malloc(sizeof(char) * 25);
    if (NULL == purl->ip)
    {
        parsed_url_free(purl);
        printk("Error on line %d (%s)\n", __LINE__, __FILE__);
        return NULL;
    }

    //TODO
    sprintf(purl->ip, "%s", " ");

    // if ((dsst = nstr2ip(purl->host)) == 0)
    // {
    //     printk("unknown host\n");
    //     parsed_url_free(purl);
    //     return NULL;
    // }
    // addr.s_addr = dsst;
    // sprintf(purl->ip, "%s", inet_ntoa(addr));
    // printk("gethostbyname ip is: %s\n", purl->ip);

    /* Set uri */
    purl->uri = (char *)url;

    /* End of the string */
    if ('\0' == *curstr)
    {
        return purl;
    }

    /* Skip '/' */
    if ('/' != *curstr)
    {
        parsed_url_free(purl);
        printk("Error on line %d (%s)\n", __LINE__, __FILE__);
        return NULL;
    }
    curstr++;

    /* Parse path */
    tmpstr = curstr;
    while ('\0' != *tmpstr && '#' != *tmpstr && '?' != *tmpstr)
    {
        tmpstr++;
    }
    len = tmpstr - curstr;
    purl->path = (char *)malloc(sizeof(char) * (len + 1));
    if (NULL == purl->path)
    {
        parsed_url_free(purl);
        printk("Error on line %d (%s)\n", __LINE__, __FILE__);
        return NULL;
    }
    (void)strncpy(purl->path, curstr, len);
    purl->path[len] = '\0';
    curstr = tmpstr;

    /* Is query specified? */
    if ('?' == *curstr)
    {
        /* Skip '?' */
        curstr++;
        /* Read query */
        tmpstr = curstr;
        while ('\0' != *tmpstr && '#' != *tmpstr)
        {
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->query = (char *)malloc(sizeof(char) * (len + 1));
        if (NULL == purl->query)
        {
            parsed_url_free(purl);
            printk("Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        (void)strncpy(purl->query, curstr, len);
        purl->query[len] = '\0';
        curstr = tmpstr;
    }
#if NOT_ONLY_SUPPORT_GET
    /* Is fragment specified? */
    if ('#' == *curstr)
    {
        /* Skip '#' */
        curstr++;
        /* Read fragment */
        tmpstr = curstr;
        while ('\0' != *tmpstr)
        {
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->fragment = (char *)malloc(sizeof(char) * (len + 1));
        if (NULL == purl->fragment)
        {
            parsed_url_free(purl);
            printk("Error on line %d (%s)\n", __LINE__, __FILE__);
            return NULL;
        }
        (void)strncpy(purl->fragment, curstr, len);
        purl->fragment[len] = '\0';
        curstr = tmpstr;
    }
#endif
    return purl;
}

/*********************************************************/