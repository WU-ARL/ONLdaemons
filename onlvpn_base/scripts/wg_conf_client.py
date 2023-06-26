#! /usr/bin/python
import sys
import os


def printUsage():
    print ('Usage: ' + sys.argv[0] + '-o <start,clear,add or del> -u <user name> -d <devname> [-p <prefix> -m <mask>]')


class CfgFileMaker:
    def __init__(self, argv):
        self.user = None
        self.dev_name = None
        self.prefix = None
        self.mask = '32'
        self.isadd = True
        self.op = None

        i = 0
        
        while ( i < len(argv)):
	    if '-u' in argv[i]:
	        i += 1
	        self.user = argv[i]
	    elif '-d' in argv[i]:
	        i += 1
	        self.dev_name = argv[i]    
	    elif '-o' in argv[i]:
	        i += 1
                self.op = argv[i]
                if 'del' in self.op:
                    self.isadd = False
	    elif '-p' in argv[i]:
	        i += 1
	        self.prefix = argv[i]    
	    elif '-m' in argv[i]:
	        i += 1
	        self.mask = argv[i]      
	    i += 1


    def isValid(self):
        if not (self.op or self.user or self.dev_name):
            return False
        if ('add' in self.op) or ('del' in self.op):
            return (self.prefix)
        return True

    def writeFile(self, wgdir):
        fnames = []
        for (dirpath, dirnames, filenames) in os.walk(wgdir):
            fnames.extend(filenames)
            break
        wgf_nm = None
        wg_base = None
        for f in fnames:
            if self.dev_name in f:
                if 'conf' in f:
                    wgf_nm = wgdir + '/' + f
                elif 'base' in f:
                    wg_base = wgdir + '/' + f
                 
                
        if not wgf_nm: #create the file
            if 'clear' in self.op:
                return True
            if wg_base:
                wgf_nm  = wgdir + '/' + self.dev_name + '.wg.conf'
                print("creating file " + wgf_nm)
                os.system('cp ' + wg_base + ' ' + wgf_nm + ' ; chown ' + self.user + '.onluser ' + wgf_nm + '; chmod 640 ' + wgf_nm)
                if 'start' in self.op:
                    return True
            else:
                print('error: there is no base file for device ' + self.dev_name + ' user:' + self.user)
                return False
        if 'clear' in self.op:
            os.system('rm ' + wgf_nm)
            return True
        if 'start' in self.op:
            if wg_base:
                wgf_nm  = wgdir + '/' + self.dev_name + '.wg.conf'
                print("creating file " + wgf_nm)
                os.system('cp ' + wg_base + ' ' + wgf_nm + ' ; chown ' + self.user + '.onluser ' + wgf_nm + '; chmod 640 ' + wgf_nm)
                return True
            else:
                print('error: there is no base file for device ' + self.dev_name + ' user:' + self.user)
                return False
        tmpf_nm = wgdir + '/tmp.' + self.dev_name + '.wg.conf'
        if 'add' in self.op:
            wg_file = open(wgf_nm, 'a')
            wg_file.write('\nAllowedIPs = ' + self.prefix + '/' + self.mask)
            wg_file.close()
            return True
            
        tmp_file = open(tmpf_nm, 'w')
        wg_file = open(wgf_nm, 'r')
        pref_mask = self.prefix + '/' + self.mask
        for line in wg_file:
            skip = False
            if (not self.isadd) and (pref_mask in line) and ('AllowedIP' in line):
                skip = True
            if not skip:
                tmp_file.write(line)
        tmp_file.close()
        wg_file.close()
        os.system('cp ' + tmpf_nm + ' ' + wgf_nm + ' ; chown ' + self.user + '.onluser ' + wgf_nm + '; chmod 640 ' + wgf_nm + "; rm " + tmpf_nm)
        return True
    

        
def main(arv=None):
    argv = sys.argv
    cfg_maker = CfgFileMaker(argv)

    if cfg_maker.isValid():
        if not cfg_maker.writeFile('/users/' + cfg_maker.user + '/.onl_wg/'):
            return 1 
    else:
        printUsage()
        return 1
    return 0
        

if __name__ == '__main__':
  status = main()
  sys.exit(status)
