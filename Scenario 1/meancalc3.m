#! /usr/bin/octave -qf 
clear;
cd /
cd /home/floris/workspace/ns-allinone-3.24/ns-3.24
measnum =800;
av = [];
stdv = [];
x =csvread('log_result_file.txt');
for k = 1 : 20
    	A{k} = x(((k-1) * measnum)+1: (k * measnum));;
	A{k} = reshape(A{k},[40,20]);
	Ar{k} = A{k}((1:k),(1:size(A{k},2)));
	av(k,1) = mean(sum(Ar{k}, 2)./sum(Ar{k}~=0, 2));
	stdv(k,1) = std(sum(Ar{k}, 2)./sum(Ar{k}~=0, 2));
end;
bar(av)
hold all;
errorbar(av,stdv,'x')
xlabel('Number of stations');
ylabel('Throughput (kb/s)');
print -djpg image2.jpg

