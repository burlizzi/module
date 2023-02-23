#include <linux/version.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(5,2,0)
#include <crypto/internal/cipher.h>
#else
#include <linux/crypto.h>
#endif
#include <linux/uaccess.h>  
#include <linux/device.h>
#include <linux/stat.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>



#include "crypt.h"
#include "config.h"










char * cryptokey=NULL;
module_param(cryptokey, charp,S_IRUGO);
MODULE_PARM_DESC(cryptokey, "crypto key (must be 16 bytes)");


struct crypto_cipher *tfm;

void crypt_init()
{
    int ret;
    if (cryptokey)
    {
        tfm = crypto_alloc_cipher("aes", 0, 0);
        if (IS_ERR(tfm)) {
            LOG(KERN_CRIT "Failed to alloc tfm for context %p\n",
                    tfm);
            cryptokey=NULL;
            return;
        }
        ret = crypto_cipher_setkey(tfm, cryptokey, strlen(cryptokey));
        if (ret) {
            LOG(KERN_CRIT "PRNG: setkey() failed flags=%x\n",
                crypto_cipher_get_flags(tfm));
            cryptokey=NULL;
        }
        LOG("crypto engine=%s\n",tfm->base.__crt_alg->cra_driver_name);
    }       
}

void crypt_done()
{
    crypto_free_cipher(tfm);
}

void encrypt(char* dest,const char* src,size_t  len)
{
    unsigned int bsize;
    unsigned int i;
    
    if (cryptokey)
    {
        bsize = crypto_cipher_blocksize(tfm);
		for (i = 0; i < len; i += bsize) {
            crypto_cipher_encrypt_one(tfm, dest+i, src+i);
        }
    }
    else
        copy_to_user(dest,src,len);

}


void decrypt(char* dest,const char* src,size_t  len)
{
    unsigned int bsize;
    unsigned int i;
    if (cryptokey)
    {
        bsize = crypto_cipher_blocksize(tfm);
		for (i = 0; i < len; i += bsize) {
            crypto_cipher_decrypt_one(tfm, dest+i, src+i);
        }
    }
    else
        copy_to_user(dest,src,len);

}