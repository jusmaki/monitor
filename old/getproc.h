extern getproc(struct procinfo *procinfo, int nproc, int sizproc);
/* struct  procinfo *procinfo;   pointer to array of procinfo struct    */
/* int  nproc;                   number of user procinfo struct */
/* int  sizproc;                 size of expected procinfo structure    */

extern getargs(struct procinfo *procinfo, int plen, char *args, int alen);
/* struct  procinfo *procinfo;   pointer to array of procinfo struct    */
/* int  plen;                    size of expected procinfo struct       */
/* char *args;                   pointer to user array for arguments    */
/* int  alen;                    size of expected argument array        */

extern getuser(struct procinfo *procinfo, int plen, void *user, int ulen);
/* struct  procinfo *procinfo;   ptr to array of procinfo struct        */
/* int     plen;                 size of expected procinfo struct       */
/* void   *user;                 ptr to array of userinfo struct, OR user */
/* int     ulen;                 size of expected userinfo struct */

