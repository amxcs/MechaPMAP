static unsigned diag_last_address, diag_last_status, diag_last_command;
static int diag_nvm_command(uint8_t command, uint16_t address, uint16_t *word)
{
    uint8_t request[4], reply[16];
    unsigned attempt;
    request[0]=(uint8_t)(address>>8);request[1]=(uint8_t)address;
    request[2]=(uint8_t)(*word>>8);request[3]=(uint8_t)*word;
    diag_last_address=address;diag_last_command=command;
    for(attempt=0;attempt<300;++attempt) {
        memset(reply,0xcc,sizeof(reply));
        if(MechaScmd(command,request,command==0x0a?2:4,reply,16)!=1) {
            diag_last_status=0x100;return 0;
        }
        diag_last_status=reply[0];
        if(reply[0]==0) {
            if(command==0x0a) *word=((uint16_t)reply[1]<<8)|reply[2];
            return 1;
        }
        if(reply[0]!=1) return 0;
        diag_pause();
    }
    return 0;
}
static int diag_read_word(uint16_t address, uint16_t *word)
{
    *word=0;
    return diag_nvm_command(0x0a,address,word);
}
static int diag_write_verified(uint16_t address, uint16_t word)
{
    uint16_t observed=0;
    if(!diag_nvm_command(0x0b,address,&word)) return 0;

    if(!diag_read_word(address,&observed)) return 0;
    if(observed!=word) {diag_last_status=0x101;return 0;}
    return 1;
}
