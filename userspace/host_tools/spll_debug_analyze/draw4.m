% Maciej Lipinski / CERN / 2014-10-22
% 
% 
% 
% options:
% 
% 
function draw2(mpll, bpll, hpll, switchover, start, finish, option)

unitScale = (1/((62.5-(62.5*((2^14)/(1+2^14))))*10^6)); % [s]
%  Xaxis     = (start:finish)*unitScale;
Xaxis     = (1:(finish-start+1))*unitScale;
to_ps     = 16000/(2^14); 

figure
subplot(4,1,1)
hold on
plot(Xaxis,mpll(start:finish,2));
plot(Xaxis,mpll(start:finish,4),'g');
plot((switchover-start+1)*unitScale,max(mpll(start:finish,2)),'*r' );
legend('raw data','long average','switchover','Location','northwest');
title('Y');
xlabel('time [s]');
ylabel('DAC out');
axis tight


%  subplot(4,1,2)
%  hold on
%  plot(Xaxis,mpll(start:finish,2),'b');
%  plot(Xaxis,smooth(mpll(start:finish,2),100),'r');
%  plot((switchover-start+1)*unitScale,max(mpll(start:finish,2)),'*r' );
%  legend('raw data','smoothed (span=100)','switchover','Location','northwest');
%  title('Y');
%  xlabel('time [s]');
%  ylabel('DAC out');
%  axis tight

subplot(4,1,2)
hold on
%  plot(Xaxis,mpll(start:finish,2),'b');
plot(Xaxis,smooth(mpll(start:finish,2),(ceil(length(Xaxis)/10))),'r');
%  plot((switchover-start+1)*unitScale,max(mpll(start:finish,2)),'*r' );

%  legend(sprintf('raw data','smoothed (span=%d)','switchover','Location','northwest',ceil(length(Xaxis)/10)));
legend(sprintf('smoothed (span=%d)',ceil(length(Xaxis)/10)));
title('Y');
xlabel('time [s]');
ylabel('DAC out');
axis tight

subplot(4,1,3)
hold on


samples=5000;
if(length(mpll) < start+samples)
  samples = length(mpll) - start;
end

plot(Xaxis(start:(start+samples)),mpll(start:(start+samples),2),'b');
plot(Xaxis(start:(start+samples)),smooth(mpll(start:(start+samples),2),10),'r');
%  plot((switchover-start+1)*unitScale,max(mpll(start:finish,2)),'*r' );
legend('raw data','smoothed (span=10)','Location','northwest');
title('Y 5000 samples');
xlabel('time [s]');
ylabel('DAC out');
axis tight

subplot(4,1,4)
hold on

samples=500;

plot(Xaxis(start:(start+samples)),mpll(start:(start+samples),2),'b');
plot(Xaxis(start:(start+samples)),smooth(mpll(start:(start+samples),2),10),'r');
%  plot((switchover-start+1)*unitScale,max(mpll(start:finish,2)),'*r' );
legend('raw data','smoothed (span=10)','Location','northwest');
title('Y 500 samples');
xlabel('time [s]');
ylabel('DAC out');
axis tight

return