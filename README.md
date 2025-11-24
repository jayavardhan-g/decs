taskset -c 3-11 python3 load_gen.py --host localhost --port 1234 --workload get_all --key-space 10000 --thread-steps 10,50,100,200,250,500,1000 --csv getpop_o.csv

python plot.py getpop.csv

htop

python3 load_gen.py --host localhost --port 1234 --thread-steps 50,200,500 --duration 30 --workload get_all --csv read_heavy.csv

python3 load_gen.py --host localhost --port 1234 --thread-steps 50,200,500 --duration 30 --workload put_all --csv write_heavy.csv

python3 load_gen.py --host localhost --port 1234 --thread-steps 50,200,500 --duration 60 --workload mixed --csv mixed_60s.csv

sudo service postgresql restart