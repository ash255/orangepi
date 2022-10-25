#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "sim.h"
#include "sunxi-scr-user.h"
#include "logger.h"
#include "tlv.h"

//FCP format
//reference: ts_102221v110100p.pdf - select chapter
//APP format
//reference: ts_102221v110100p.pdf - Coding of an application template entry 
#define TAG_FCP         0x62
#define TAG_FILE_SIZE   0x80
#define TAG_APP         0x61
#define TAG_AID         0x4F

#define select_fid(presp, fid) sim_wrdata_type2(presp, CLA_3G_NORMAL, INS_SELECT, 0x00, 0x04, 2, fid)
#define select_aid(presp, aid) sim_wrdata_type2(presp, CLA_3G_NORMAL, INS_SELECT, 0x04, 0x04, 16, aid)
#define read_binary(presp, len) sim_wrdata_type1(presp, CLA_3G_NORMAL, INS_READ_BINARY, 0x00, 0x00, len)
#define read_record(presp, num, len) sim_wrdata_type1(presp, CLA_3G_NORMAL, INS_READ_RECORD, num, 0x04, len)
#define get_response(presp, len) sim_wrdata_type1(presp, CLA_3G_NORMAL, INS_GET_RESPONSE, 0x00, 0x00, len)
#define status(presp, len) sim_wrdata_type1(presp, CLA_3G_SPECIAL, INS_STATUS, 0x00, 0x00, len)
#define authenticate(presp, data) sim_wrdata_type2(presp, CLA_3G_NORMAL, INS_AUTHENTICATE, 0x00, 0x00, 0x22, data)

static int g_fd;

/* type1: CLS + INS + P1 + P2 + le  -> only read, le=read size*/
int sim_wrdata_type1(struct sim_resp_t *presp, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t le)
{
    uint8_t sbuffer[5] = {0};
    struct scr_wr_data wr_data = {0};
    
    sbuffer[0] = cla;
    sbuffer[1] = ins;
    sbuffer[2] = p1;
    sbuffer[3] = p2;
    sbuffer[4] = le;
    
    wr_data.cmd_buf = sbuffer;
    wr_data.cmd_len = sizeof(sbuffer);
    wr_data.rtn_data = presp->resp;
    wr_data.rtn_len = &presp->len;  
    wr_data.psw1 = &presp->sw1;
    wr_data.psw2 = &presp->sw2;

    __LOG_DEBUG_ARRAY__(wr_data.cmd_buf, wr_data.cmd_len);
    int ret = ioctl(g_fd, SCR_IOCWRDATA, &wr_data);
    if(0 > ret)
    {
        presp->status = 0;
        __LOG_ERROR__("sim write-read failed, ret=%d\n", ret);
        return 0;
    }
    
    __LOG_DEBUG__("sw1=%02X sw2=%02X\n", presp->sw1, presp->sw2);
    __LOG_DEBUG_ARRAY__(presp->resp, presp->len);  
    presp->status = 1;
    return 1;
}

/* type2: CLS + INS + P1 + P2 + lc + data -> only lc, write data, lc=data size*/
int sim_wrdata_type2(struct sim_resp_t *presp, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t lc, uint8_t *data)
{
    uint8_t sbuffer[256] = {0};
    struct scr_wr_data wr_data = {0};
    
    sbuffer[0] = cla;
    sbuffer[1] = ins;
    sbuffer[2] = p1;
    sbuffer[3] = p2;
    sbuffer[4] = lc;
    memcpy(&sbuffer[5], data, lc);
    
    wr_data.cmd_buf = sbuffer;
    wr_data.cmd_len = lc + 5;
    wr_data.rtn_data = presp->resp;
    wr_data.rtn_len = &presp->len;  
    wr_data.psw1 = &presp->sw1;
    wr_data.psw2 = &presp->sw2;

    __LOG_DEBUG_ARRAY__(wr_data.cmd_buf, wr_data.cmd_len);
    int ret = ioctl(g_fd, SCR_IOCWRDATA, &wr_data);
    if(0 > ret)
    {
        presp->status = 0;
        __LOG_ERROR__("sim write-read failed, ret=%d\n", ret);
        return 0;
    }
    __LOG_DEBUG__("sw1=%02X sw2=%02X\n", presp->sw1, presp->sw2);
    __LOG_DEBUG_ARRAY__(presp->resp, presp->len);  
    presp->status = 1;
    return 1;   
}

/* type3: CLS + INS + P1 + P2 + lc + data +le -> le+lc  */
int sim_wrdata_type3(struct sim_resp_t *presp, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t lc, uint8_t *data, uint8_t le)
{
    uint8_t sbuffer[256] = {0};
    struct scr_wr_data wr_data = {0};
    
    sbuffer[0] = cla;
    sbuffer[1] = ins;
    sbuffer[2] = p1;
    sbuffer[3] = p2;
    sbuffer[4] = lc;
    memcpy(&sbuffer[5], data, lc);
    sbuffer[lc + 5] = le;
    
    wr_data.cmd_buf = sbuffer;
    wr_data.cmd_len = lc + 6;
    wr_data.rtn_data = presp->resp;
    wr_data.rtn_len = &presp->len;  
    wr_data.psw1 = &presp->sw1;
    wr_data.psw2 = &presp->sw2;

    __LOG_DEBUG_ARRAY__(wr_data.cmd_buf, wr_data.cmd_len);
    int ret = ioctl(g_fd, SCR_IOCWRDATA, &wr_data);
    if(0 > ret)
    {
        presp->status = 0;
        __LOG_ERROR__("sim write-read failed, ret=%d\n", ret);
        return 0;
    }
    __LOG_DEBUG__("sw1=%02X sw2=%02X\n", presp->sw1, presp->sw2);
    __LOG_DEBUG_ARRAY__(presp->resp, presp->len);  
    presp->status = 1;
    return 1;   
}

static uint8_t* strtohex(char *str)
{
    static uint8_t buffer[32] = {0};
    int i, len = sizeof(buffer);
    char chars[3] = {0};
    
    if(len > (strlen(str)/2))
        len = (strlen(str)/2);
   
    for(i=0;i<len;i++)
    {
        chars[0] = str[i*2];
        chars[1] = str[i*2+1];
        buffer[i] = strtol(chars, NULL, 16);
    }
    return buffer;
}

static void sim_info(int fd)
{
    int ret;
    struct scr_card_para scp = {0};
    
    // ret = ioctl(fd, SCR_IOCGPARA, &scp);
    // __LOG_DEBUG__("card param f=%d\n", scp.f);
    // __LOG_DEBUG__("card param d=%d\n", scp.d);
    // __LOG_DEBUG__("card param freq=%d\n", scp.freq);
    // __LOG_DEBUG__("card param recv_no_parity=%d\n", scp.recv_no_parity);
    // __LOG_DEBUG__("card param protocol_type=%d\n", scp.protocol_type);
      
    struct scr_atr atr = {0};
    ret = ioctl(fd, SCR_IOCGATR, &atr);
    
    __LOG_DEBUG__("ATR len: %d\n", atr.atr_len);
    __LOG_DEBUG_ARRAY__(atr.atr_data, atr.atr_len);

    // struct smc_atr_para atrp = {0};
    // ret = ioctl(fd, SCR_IOCGATRPARA, &atrp);
    // __LOG_DEBUG__("atr TS=%d\n", atrp.TS);
    // __LOG_DEBUG_ARRAY__(atrp.TK, sizeof(atrp.TK));
    // __LOG_DEBUG__("atr TK_NUM=%d\n", atrp.TK_NUM);
    // __LOG_DEBUG__("atr T=%d\n", atrp.T);
    // __LOG_DEBUG__("atr F=%d\n", atrp.F); 
    // __LOG_DEBUG__("atr D=%d\n", atrp.D); 
    // __LOG_DEBUG__("atr I=%d\n", atrp.I); 
    // __LOG_DEBUG__("atr P=%d\n", atrp.P); 
    // __LOG_DEBUG__("atr N=%d\n", atrp.N); 
    
    struct smc_pps_para ppsp = {0};
    ret = ioctl(fd, SCR_IOCGPPSPARA, &ppsp);
    
    __LOG_DEBUG__("PTS len: %d\n", sizeof(struct smc_pps_para));
    __LOG_DEBUG_ARRAY__((char*)&ppsp, sizeof(struct smc_pps_para));   
   
    // __LOG_DEBUG__("pps ppss=%d\n", ppsp.ppss);
    // __LOG_DEBUG__("pps pps0=%d\n", ppsp.pps0);
    // __LOG_DEBUG__("pps pps1=%d\n", ppsp.pps1);
    // __LOG_DEBUG__("pps pps2=%d\n", ppsp.pps2); 
    // __LOG_DEBUG__("pps pps3=%d\n", ppsp.pps3); 
    // __LOG_DEBUG__("pps pck=%d\n", ppsp.pck);

}

static void print_resp(struct sim_resp_t *presp)
{
    if(presp->status == 1)
    {
        __LOG_MESSAGE__("sw1=%02X\n", presp->sw1);
        __LOG_MESSAGE__("sw2=%02X\n", presp->sw2);
        __LOG_MESSAGE_ARRAY__(presp->resp, presp->len);       
    }

}

static void param_split(char *cmd, int *parc, char **parv)
{
    int param_num = 0;
    
    parv[param_num] = cmd;
    char *temp = strtok(cmd, " ");
    while(temp != NULL)
    {
        parv[param_num++] = temp;
        temp = strtok(NULL, " ");
    }
    
    *parc = param_num;
}

static void cmd_proc()
{
    struct sim_resp_t resp = {0};
    char buffer[256] = {0};
    int parc;
    char *parv[16];
    while(1)
    {
        parc = 0;
        resp.len = 0;
        resp.status = 0;
        memset(buffer, 0, sizeof(buffer));
        memset(parv, 0, 16*sizeof(char*));
        printf(">");
        
        if(fgets(buffer, sizeof(buffer), stdin) == NULL)
            break;
        if(strlen(buffer) <= 0)
            continue;
        if(buffer[strlen(buffer)-1] == '\n')
            buffer[strlen(buffer)-1] = '\x00';
        
        param_split(buffer, &parc, parv);
        if(parc == 0)
            continue;
        
        if(strcmp(parv[0], "sf") == 0 && parc == 2)    //select file: fid/aid
        {
            if(strlen(parv[1]) > 4)
                select_aid(&resp, strtohex(parv[1]));
            else if(strlen(parv[1]) > 0)
                select_fid(&resp, strtohex(parv[1]));
            print_resp(&resp);
        }else if(strcmp(parv[0], "rb") == 0 && parc == 2) //read binary: expect len
        {
            int exlen = strtol(parv[1], NULL, 10);
            read_binary(&resp, exlen);
            print_resp(&resp);          
        }else if(strcmp(parv[0], "rr") == 0 && parc == 3) //read record: record num, expect len
        {
            int rnum = strtol(parv[1], NULL, 10);
            int exlen = strtol(parv[2], NULL, 10);
            read_record(&resp, rnum, exlen);
            print_resp(&resp);             
        }else if(strcmp(parv[0], "gr") == 0 && parc >= 1) //get response: expect len
        {
            if(parc == 1 && resp.sw2 > 0)
            {
                get_response(&resp, resp.sw2);
                print_resp(&resp);                
            }else if(parc == 2)
            {
                int exlen = strtol(parv[1], NULL, 10);
                get_response(&resp, exlen);
                print_resp(&resp);               
            }            
        }else if(strcmp(parv[0], "status") == 0 && parc >= 1) //status: expect len
        {
            if(parc == 1 && resp.sw2 > 0)
            {
                status(&resp, resp.sw2);
                print_resp(&resp);
            }else if(parc == 2)
            {
                int exlen = strtol(parv[1], NULL, 10);
                status(&resp, exlen);
                print_resp(&resp);                
            }           
        }else if(strcmp(parv[0], "auth") == 0 && parc == 3) //authenticate: rand(16bytes) autn(16bytes)
        {
            uint8_t auth_data[0x22] = {0};
            auth_data[0] = 0x10;
            memcpy(&auth_data[1], strtohex(parv[1]), 16);
            auth_data[0x11] = 0x10;
            memcpy(&auth_data[0x12], strtohex(parv[2]), 16);
            authenticate(&resp, auth_data);
            print_resp(&resp);                
        }else if(strcmp(parv[0], "ex") == 0)
        {
            break;
        }
    }   
}

static int get_aid(uint8_t *aid)
{
    struct sim_resp_t resp = {0};
    struct tlv_t *tlv = NULL, *tlv_size = NULL, *tlv_aid = NULL;
    
    //select adf
    //reference: https://www.jianshu.com/p/200e10215c8d
    select_fid(&resp, strtohex("3F00"));
    select_fid(&resp, strtohex("2F00"));
    get_response(&resp, resp.sw2);
    if(resp.resp[0] != TAG_FCP)
    {
        __LOG_ERROR__("response is not FCP format\n");
        goto __GET_AID_FAILED;
    }
    
    //*****************************************
    //************   FCP FORMAT   *************
    //*****************************************
    //tag  |  len  |  template
    //62   |  1F   |  82  05  42  21  ......
    tlv = tlv_parser(&resp.resp[2], resp.len-2);
    if(tlv == NULL)
    {
        __LOG_ERROR__("FCP tlv parse failed\n");
        goto __GET_AID_FAILED;
    }
    tlv_size = tlv_get(tlv, TAG_FILE_SIZE);
    if(tlv_size == NULL)
    {
        __LOG_ERROR__("get file size failed\n");
        goto __GET_AID_FAILED;
    }

    //*****************************************
    //************   TLV FORMAT   *************
    //*****************************************
    //tag  |  len  |  value
    //80   |  02   |  00  2A
    if(read_record(&resp, 1, tlv_size->value[1]) == 0)
    {
        __LOG_ERROR__("get AID record failed\n");
        goto __GET_AID_FAILED;
    }
    tlv_free(tlv);
    tlv = NULL;
    
    if(resp.resp[0] != TAG_APP)
    {
        __LOG_ERROR__("response is not APP format\n");
        goto __GET_AID_FAILED;
    }
    
    tlv = tlv_parser(&resp.resp[2], resp.len-2);
    if(tlv == NULL)
    {
        __LOG_ERROR__("APP tlv parse failed\n");
        goto __GET_AID_FAILED;
    }
    
    tlv_aid = tlv_get(tlv, TAG_AID);
    if(tlv_aid == NULL)
    {
        __LOG_ERROR__("get aid from tlv failed\n");
        goto __GET_AID_FAILED;
    }
    
    if(tlv_aid->len != 16)
    {
        __LOG_ERROR__("AID len is not 16 bytes\n");
        goto __GET_AID_FAILED;
    }

    memcpy(aid, tlv_aid->value, 16);
    tlv_free(tlv);
    return 1;
    
__GET_AID_FAILED:
    if(tlv != NULL)
        tlv_free(tlv);
    return 0;
}

static int get_imsi(uint8_t *imsi)
{
    struct sim_resp_t resp = {0};
    struct tlv_t *tlv = NULL, *tlv_size = NULL;
    
    select_fid(&resp, strtohex("3F00"));
    select_fid(&resp, strtohex("7F20"));
    select_fid(&resp, strtohex("6F07"));
    
    if(get_response(&resp, resp.sw2) == 0)
    {
        __LOG_ERROR__("get response failed\n");
        goto __GET_IMSI_FAILED;
    }
    
    tlv = tlv_parser(&resp.resp[2], resp.len-2);
    tlv_size = tlv_get(tlv, TAG_FILE_SIZE);
    if(tlv_size == NULL)
    {
        __LOG_ERROR__("get IMSI attribute failed\n");
        goto __GET_IMSI_FAILED;
    }
    
    if(read_binary(&resp, tlv_size->value[1]) == 0)
    {
        __LOG_ERROR__("get IMSI failed\n");
        goto __GET_IMSI_FAILED;
    }
    if(resp.len != 9)
    {
        __LOG_ERROR__("IMSI len is not 9 bytes\n");
        goto __GET_IMSI_FAILED;
    }
    
    imsi[0] = resp.resp[1] >> 4;
    imsi[1] = (resp.resp[2] >> 4) | (resp.resp[2] << 4);
    imsi[2] = (resp.resp[3] >> 4) | (resp.resp[3] << 4);
    imsi[3] = (resp.resp[4] >> 4) | (resp.resp[4] << 4);
    imsi[4] = (resp.resp[5] >> 4) | (resp.resp[5] << 4);
    imsi[5] = (resp.resp[6] >> 4) | (resp.resp[6] << 4);
    imsi[6] = (resp.resp[7] >> 4) | (resp.resp[7] << 4);
    imsi[7] = (resp.resp[8] >> 4) | (resp.resp[8] << 4);
    
    tlv_free(tlv);
    tlv = NULL;
    return 1;
    
__GET_IMSI_FAILED:
    if(tlv != NULL)
        tlv_free(tlv);
    return 0;
}

static int get_iccid(uint8_t *iccid)
{
    struct sim_resp_t resp = {0};
    struct tlv_t *tlv = NULL, *tlv_size = NULL;
    
    select_fid(&resp, strtohex("3F00"));
    select_fid(&resp, strtohex("2FE2"));
    
    if(get_response(&resp, resp.sw2) == 0)
    {
        __LOG_ERROR__("get response failed\n");
        goto __GET_ICCID_FAILED;
    }
    
    tlv = tlv_parser(&resp.resp[2], resp.len-2);
    tlv_size = tlv_get(tlv, TAG_FILE_SIZE);
    if(tlv_size == NULL)
    {
        __LOG_ERROR__("get ICCID attribute failed\n");
        goto __GET_ICCID_FAILED;
    }
    
    if(read_binary(&resp, tlv_size->value[1]) == 0)
    {
        __LOG_ERROR__("get ICCID failed\n");
        goto __GET_ICCID_FAILED;
    }
    if(resp.len != 10)
    {
        __LOG_ERROR__("ICCIC len is not 10 bytes\n");
        goto __GET_ICCID_FAILED;
    }
    
    memcpy(iccid, resp.resp, 10);
  
    //reverse bytes
    int i;
    for(i=0;i<10;i++)
        iccid[i] = (iccid[i] >> 4) | (iccid[i] << 4);
    
    tlv_free(tlv);
    tlv = NULL;
    return 1;
    
__GET_ICCID_FAILED:
    if(tlv != NULL)
        tlv_free(tlv);
    return 0;
}

static int work()
{
    uint8_t imsi[8] = {0};
    uint8_t iccid[10] = {0};
    uint8_t aid[16] = {0};
    struct sim_resp_t resp = {0};
    
    if(get_iccid(iccid) == 0)
    {
        __LOG_ERROR__("get iccid failed\n");
        return 0;       
    }  
    
    if(get_imsi(imsi) == 0)
    {
        __LOG_ERROR__("get imsi failed\n");
        return 0;       
    }
    
    if(get_aid(aid) == 0)
    {
        __LOG_ERROR__("get aid failed\n");
        return 0;
    }
    
    __LOG_MESSAGE__("ICCID=%02X%02X%02X%02X%02X%02X%02X%02X\n", iccid[0],iccid[1],iccid[2],
        iccid[3],iccid[4],iccid[5],iccid[6],iccid[7],iccid[8],iccid[9]);
    
    __LOG_MESSAGE__("IMSI=%X%02X%02X%02X%02X%02X%02X%02X\n", 
        imsi[0],imsi[1],imsi[2],imsi[3],imsi[4],imsi[5],imsi[6],imsi[7]);
    
    __LOG_MESSAGE_ARRAY__(aid, 16);
    
    select_aid(&resp, aid);
    //authenticate(&resp, );
    
    return 1;
}

static int traverse_efs()
{
    struct sim_resp_t resp = {0};
    select_fid(&resp, strtohex("3F00"));
    select_fid(&resp, strtohex("7F20"));
    select_fid(&resp, strtohex("2F00"));
    //sim_wrdata_type2(&resp, CLA_3G_NORMAL, INS_SELECT, 0x01, 0x01, 2, strtohex("7F20"));
    
    if(resp.sw1 == 0x61)
    {
        get_response(&resp, resp.sw2);
    }
    
}

int main(int argc, char **argv)
{
    log_with_fd(stdout);
    //log_set_level(__LEVEL_DEBUG__);
    log_set_level(__LEVEL_MESSAGE__);
    log_set_level(__LEVEL_ERROR__);
    
    int i, ret, card_status;
    g_fd = open("/dev/smartcard", O_RDWR);
    if (-1 == g_fd)
    {
        __LOG_ERROR__("open /dev/driver1 failed! \n");
        return 0;
    }
    
    ret = ioctl(g_fd, SCR_IOCGSTATUS, &card_status);
    if(0 == card_status)
    {
        __LOG_ERROR__("card not insert, status=%d\n", card_status);
        close(g_fd);
        return 0;
    }
 
    int mode = 0;   //0: work  1: command
    if(argc >= 2)
    {
        for(i=1;i<argc;i++)
        {
            if(strcmp(argv[i], "-r") == 0)
            {
                ret = ioctl(g_fd, SCR_IOCRESET);
                if(ret < 0)
                {
                    __LOG_ERROR__("card reset failed\n");
                    close(g_fd);
                    return 0;                   
                }
                __LOG_MESSAGE__("card reset successful\n");
            }else if(strcmp(argv[i], "-c") == 0)
            {
                mode = 1;
            }else if(strcmp(argv[i], "-d") == 0)
            {
                log_set_level(__LEVEL_DEBUG__);
            }else if(strcmp(argv[i], "-h") == 0)
            {
                printf("%s [-rchd]\n", argv[0]);
                printf("-c command mode\n");
                printf("-r reset card\n");
                printf("-d show debug log\n");
                printf("-h print help\n");
            }
        }
    }
    sim_info(g_fd);
   
    if(mode == 1)
    {
        cmd_proc();
    }else
    {
        // work();
        traverse_efs();
    }

    close(g_fd);
    return 1;
}