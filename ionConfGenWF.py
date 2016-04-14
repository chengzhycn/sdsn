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
import csv

def gen_ipnadmin(rc_file, config):
    with open(rc_file, 'a') as f:
        f.write('## begin ipnadmin\n')
        for key, value in config.items():
            f.write('a plan %s ltp/%s\n' % (key, key))
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

def gen_bpadmin(rc_file, host, config):
    with open(rc_file, 'a') as f:
        f.write('## begin bpadmin' + '\n' + '1\n')
        f.write("a scheme ipn 'ipnfw' 'ipnadminep'\n")
        f.write('a endpoint ipn:%s.1 q\n' % host)
        f.write('a endpoint ipn:%s.2 q\n' % host)
        f.write('a protocol ltp 1400 100\n')
        f.write('a induct ltp 1 ltpcli\n')
        for key, value in config.items():
            f.write('a outduct ltp %s ltpclo\n' % key)
        f.write('s' + '\n' + '## end bpadmin\n')
        f.write('\n')

def gen_ltpadmin(rc_file, host, config):
    with open(rc_file, 'a') as f:
        f.write('## begin ltpadmin\n')
        f.write('1 100\n')
        for key, value in config.items():
            f.write("a span %s 100 100 1300 1400 1 'udplso' %s:1113 1000000'\n" % (key, value))
        f.write("s 'udplsi " + config.get(str(host)) + ":1113'\n")
        f.write("## end ltpadmin\n")
        f.write("\n")

def gen_ionadmin(rc_file, matrix, row, column):
    host = matrix[row][column]
    with open(rc_file, 'a') as f:
        f.write('## begin ionadmin\n')
        f.write("1 %s '/root/test/dtn.ionconfig'\n" % host)
        f.write('s\n')
        f.write('m horizon +0\n')
        for i in matrix:
            for j in i:
                f.write('a contact +1 +86400 %s %s 1000000\n' % (j, j))
        f.write('\n')
        for i in matrix:
            for j in i:
                f.write('a range +1 +86400 %s %s 0\n' % (j, j))
        f.write('\n')

        found_node = []
        dfs_find(matrix, row, column, found_node, 3, f)

        f.write('m production 1000000\n')
        f.write('m consumption 1000000\n')
        f.write('## end ionadmin\n')
        f.write('\n')

def gen_file(rc_file, config):
    if os.path.exists(rc_file):
        print ("Warning: %s has existed and will be rewritten!!!" % rc_file)
        opt = raw_input("To be continue?(y/n)")
        if opt == 'n':
            os._exit(0)

    with open(rc_file, 'w') as f:
        f.write('# ION CONFIGURATION\n')
        for key, value in config.items():
            f.write('# host%s %s\n' % (key, value))
        f.write('\n')
       
def load_config(config, parser):
    for name, value in parser.items('config'):
        config[name] = value
    return config

def sort_items(config):
    return sorted(config.items(), key = lambda d:d[0], reverse = False)

def matrix_process(matrix_path):
    """
    matrix = matrix.strip().split(',')
    array = []
    for item in matrix:
        item = item.strip().split(' ')
        array.append(map(int, item))
    return array
    """
    try:
        with open(matrix_path, 'rt') as fin:
            cin = csv.reader(fin)
            matrix = [map(int, row) for row in cin]
    except Exception as e:
        print ("ERROR: cannot locate matrix file: %s" % e)
    return matrix

def dfs_find(matrix, row, column, found_node, deepth, f):
    print "row=%d column=%d matrix[row][column]=%d" % (row, column, matrix[row][column])
    if found_node.count(matrix[row][column]) != 0:
        return -1;
    if deepth == 0:
        return matrix[row][column]

    found_node.append(matrix[row][column])

    if column != 0:
        matrix_l = dfs_find(matrix, row, column-1, found_node, deepth-1, f)
        dfs_print(matrix[row][column], matrix_l, f)
    if column != len(matrix[row])-1:
        matrix_r = dfs_find(matrix, row, column+1, found_node, deepth-1, f)
        dfs_print(matrix[row][column], matrix_r, f)
    matrix_u = dfs_find(matrix, (row+1)%len(matrix), column, found_node, deepth-1, f)
    dfs_print(matrix[row][column], matrix_u, f)
    matrix_d = dfs_find(matrix, (row-1)%len(matrix), column, found_node, deepth-1, f)
    dfs_print(matrix[row][column], matrix_d, f)

    return matrix[row][column]

def dfs_print(node_1, node_2, f):
    if node_2 >= 0:
        f.write('a contact +1 +86400 %d %d 1000000\n' % (node_1, node_2))
        f.write('a contact +1 +86400 %d %d 1000000\n' % (node_2, node_1))
        f.write('a range +1 +86400 %d %d 1\n' % (node_1, node_2))
        f.write('\n')

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option("-c", "--config", dest="config",
                    help="config files")
    parser.add_option("-m", "--matrix", dest="matrix",
                    help="connection matrix, with file format in csv")
    #parser.add_option("-p", "--protocol", dest="protocol",
    #                help="udp/ltp")
    (options, args) = parser.parse_args()

    config_path = options.config
    matrix_path = options.matrix
    #protocol = options.protocol

    read_config = configparser.ConfigParser(delimiters='=')
    read_config.read(config_path)
    config = {}
    config = load_config(config, read_config)
    num = len(config)
    sort_config = sort_items(config)

    matrix = matrix_process(matrix_path)

    for row in range(len(matrix)):
        for column in range(len(matrix[row])):
            filename = 'host%s.rc' % matrix[row][column]
            gen_file(filename, config)
            gen_ionadmin(filename, matrix, row, column) 
            gen_ltpadmin(filename, matrix[row][column], config)
            gen_bpadmin(filename, matrix[row][column], config)
            gen_cfdpadmin(filename)
            gen_ipnadmin(filename, config)
