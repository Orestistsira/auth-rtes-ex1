clearvars;
close all;

data = importdata('stats.txt');

m = mean(data, 2);

figure
plot(1:length(m), m);
xlabel('number of consumers');
ylabel('mean waiting time (us)');

figure
histogram(data(4, :));
xlabel('waiting time (us)');
ylabel('jobs');
xlim([0 50]);