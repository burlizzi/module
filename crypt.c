#include <linux/crypto.h>
#include <linux/uaccess.h>  
#include <linux/device.h>
#include <asm/i387.h>
#include <asm/crypto/aes.h>
#include <linux/stat.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>



#include "crypt.h"
#include "config.h"








void (*aesni_enc)(struct crypto_aes_ctx *ctx, u8 *out,const u8 *in);
void (*aesni_dec)(struct crypto_aes_ctx *ctx, u8 *out,const u8 *in);


char * cryptokey=NULL;
module_param(cryptokey, charp,S_IRUGO);
MODULE_PARM_DESC(cryptokey, "crypto key");


struct crypto_tfm tfm;

struct crypto_aes_ctx *ctx;

void crypt_init()
{
    aesni_enc=kallsyms_lookup_name("aesni_enc");
    aesni_dec=kallsyms_lookup_name("aesni_dec");
    LOG("vrfm: aesni_enc=%p aesni_dec=%p\n",aesni_enc,aesni_dec);
    ctx = crypto_tfm_ctx(&tfm);
    if (cryptokey)
        crypto_aes_set_key(&tfm, cryptokey, AES_KEYSIZE_256);
        
}

void crypt_done()
{

}

void encrypt(char* dest,const char* src,size_t  len)
{
    if (cryptokey)
    {
        //AES_encrypt(&tfm,dest,src);
        
        if (irq_fpu_usable() && aesni_enc)
        {
            kernel_fpu_begin();
            aesni_enc(ctx, dest, src);
            kernel_fpu_end();
        } 
        else
            crypto_aes_encrypt_x86(ctx, dest, src);

        /**/   
    }
    else
        memcpy(dest,src,len);

}


void decrypt(char* dest,const char* src,size_t  len)
{
    if (cryptokey)
    {
        //AES_decrypt(&tfm,dest,src);
        
        if (irq_fpu_usable() && aesni_dec)
        {
            kernel_fpu_begin();
            aesni_dec(ctx, dest, src);
            kernel_fpu_end();
        }
        else 
            crypto_aes_decrypt_x86(ctx, dest, src);
        
    }
    else
        memcpy(dest,src,len);

}