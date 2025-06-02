% AERO_GUID_PLOT StopFcn plot script for Simulink model 'guidance'

%   J.Hodgson  
%   Copyright 1990-2014 The MathWorks, Inc.

d2r = pi/180;

if ~isempty(findobj(0,'Tag','F3'));delete(findobj(0,'Tag','F3'));end;
f3 = figure('Tag','F3');
set(gcf,'pos',[408 61 596 569])

subplot(221);plot(Latax.time,Latax.signals.values(:,1)/9.81,Latax.time,Latax.signals.values(:,2)/9.81,'-.');grid;
set(gca,'xlim',[0 max(Latax.time)])
xlabel('时间 [Sec]');ylabel('法向加速度 [g]')
legend('a_z','a_{zdemand}','Location','SouthEast')
subplot(222);plot(Incid.time,Incid.signals.values/d2r);grid
set(gca,'xlim',[0 max(Incid.time)])
xlabel('时间 [Sec]');ylabel('迎角 [deg]')
subplot(223);plot(Mach.time,Mach.signals.values);grid
set(gca,'xlim',[0 max(Mach.time)])
xlabel('时间 [Sec]');ylabel('马赫数')
subplot(224);plot(Fin_dem.time,Fin_dem.signals(1).values/d2r);grid;
set(gca,'xlim',[0 max(Fin_dem.time)])
xlabel('时间 [Sec]');ylabel('舵面偏转量 [deg]')

if ~isempty(findobj(0,'Tag','F4'));delete(findobj(0,'Tag','F4'));end;
f4 = figure('Tag','F4');
set(gcf,'pos',[10 60 390 250])
I=find(diff(Mode.signals.values)~=0);
plot(Gimbal.time,Gimbal.signals.values(:,2)/d2r,'--',Gimbal.time,Gimbal.signals.values(:,1)/d2r,Gimbal.time(I),Gimbal.signals.values(I,1)/d2r,'rx');grid;
set(gca,'xlim',[0 max(Gimbal.time)],'ylim',[-30 30])
legend('实际目标角度','导引头相对于导弹纵轴的旋转角度','模式切换点')
xlabel('时间 [Sec]');ylabel('旋转角度到目标的角度 [deg]')

if ~isempty(findobj(0,'Tag','F5'));delete(findobj(0,'Tag','F5'));end;
f5 = figure('Tag','F5');
set(gcf,'pos',[10 385 390 244])
h=plot(Tgt_pos.signals.values(:,1),Tgt_pos.signals.values(:,2),'--',Miss_pos.signals.values(:,1),Miss_pos.signals.values(:,2),Miss_pos.signals.values(end,1),Miss_pos.signals.values(end,2),'x');grid
set(gca,'ydir','reverse')
xlabel('X [m]');ylabel('Z [m]');title('镖体和目标轨迹');
legend('目标','镖体','Location','NorthWest')
