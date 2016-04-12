#!/usr/bin/python2.7
# This is a ion config file auto-gen script. To use this script, you should 
# edit a config file, add ion ip address and connection matrix just like the 
# example config file written.
#
# Author: chengzhycn
# Email:  chengzhycn@gmail.com
# Date:   2016-04-12


import os
import optparse
import configparser

def gen_ipnadmin(rc_file, num):
    with open(rc_file, 'a') as f:
        f.write('## begin ipnadmin\n')
        for i in range(1, num+1):
            f.write('a plan %s ltp/%s\n' % (i, i))
        f.write('## end ipnadmin\n')
        f.write('\n')

def gen_cfdpadmin(rc_file):
    with open(rc_file, 'a') as f:
        f.write('## begin cfdpadmin\n')
        f.write('1\n')
        f.write('e 1\n')
        f.write('m discard 0\n')
        f.write('m requirecrc 1\n')
        f.write('m segsize 32768\n')
        f.write("s 'bputa'\n")
        f.write('## end cfdpadmin\n')
        f.write('\n')

def gen_bpadmin(rc_file, num, host):
    with open(rc_file, 'a') as f:
        f.write('## begin bpadmin' + '\n' + '1\n')
        f.write("a scheme ipn 'ipnfw' 'ipnadminep'\n")
        f.write('a endpoint ipn:%s.1 q\n' % host)
        f.write('a endpoint ipn:%s.2 q\n' % host)
        f.write('a protocol ltp 1400 100\n')
        f.write('a induct ltp 1 ltpcli\n')
        for i in range(1, num+1):
            f.write('a outduct ltp %s ltpclo\n' % i)
        f.write('s' + '\n' + '## end bpadmin\n')
        f.write('\n')

def gen_ltpadmin(rc_file, num, host, sort_config):
    with open(rc_file, 'a') as f:
        f.write('## begin ltpadmin\n')
        f.write('1 100\n')
        for item in sort_config:
            f.write("a span %s 100 100 1300 1400 1 'udplso' %s:1113 1000000'\n" % (item[0], item[1]))
        f.write("s 'udplsi " + sort_config[host-1][1] + ":1113'\n")
        f.write("## end ltpadmin\n")
        f.write("\n")

def gen_ionadmin(rc_file, num, host, matrix):
    with open(rc_file, 'a') as f:
        f.write('## begin ionadmin\n')
        f.write("1 %s '/root/test/dtn.ionconfig'\n" % host)
        f.write('s\n')
        f.write('m horizon +0\n')
        for i in range(1, num+1):
            f.write('a contact +1 + 86400 %s %s 1000000\n' % (i, i))
        f.write('\n')
        for i in range(len(matrix)):
            for j in range(len(matrix[i])):
                if matrix[i][j]:
                    f.write('a contact +1 +86400 %d %d 1000000\n' % (i+1, j+1))

        f.write('\n')
        for i in range(1, num+1):
            f.write('a range +1 + 86400 %s %s 0\n' % (i, i))
        f.write('\n')
        for i in range(len(matrix)):
            for j in range(len(matrix[i])):
                if matrix[i][j] and i < j:
                    f.write('a range +1 +86400 %d %d %d\n' % (i+1, j+1, matrix[i][j]))

        f.write('\n')
        f.write('m production 1000000\n')
        f.write('m consumption 1000000\n')
        f.write('## end ionadmin\n')
        f.write('\n')

def gen_file(rc_file, sort_config):
    if os.path.exists(rc_file):
        print ("Warning: %s has existed and will be rewritten!!!" % rc_file)
        print ("To be continue?(y/n)")
        opt = input()
        if opt == n:
            os._exit()

    with open(rc_file, 'w') as f:
        f.write('# ION CONFIGURATION\n')
        for item in sort_config:
            f.write('# host%s %s\n' % (item[0], item[1]))
        f.write('\n')
       
def load_config(config, parser):
    for name, value in parser.items('config'):
        config[name] = value
    return config

def sort_items(config):
    return sorted(config.items(), key = lambda d:d[0], reverse = False)

def matrix_process(matrix):
    matrix = matrix.strip().split(',')
    array = []
    for item in matrix:
        item = item.strip().split(' ')
        array.append(map(int, item))
    return array

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option("-c", "--config", dest="config",
                    help="config files")
    (options, args) = parser.parse_args()

    config_path = options.config

    read_config = configparser.ConfigParser(delimiters='=')
    read_config.read(config_path)
    config = {}
    config = load_config(config, read_config)

    matrix = matrix_process(config['matrix'])
    del config['matrix']
    num = len(config)
    sort_config = sort_items(config)

    for i in range(1, num+1):
        filename = 'host%s.rc' % i
        gen_file(filename, sort_config)
        gen_ionadmin(filename, num, i, matrix) 
        gen_ltpadmin(filename, num, i, sort_config)
        gen_bpadmin(filename, num, i)
        gen_cfdpadmin(filename)
        gen_ipnadmin(filename, num)
