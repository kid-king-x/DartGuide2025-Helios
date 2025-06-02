clear all
clc
clear
tt=0.1; %时间步长
sm=0.7*tt; %追踪者每次移动的距离
st=0.32*tt;%目标每次移动的距离
x(1)=0;y(1)=0;z(1)=0;%追踪者（导弹）的初始位置
pmr(:,1)=[x(1);y(1);z(1)];%追踪者的位置向量,position of the missile/追踪者
ptr(:,1)=[25;5;10];%目标的位置向量,position of the target/目标
m=4;%比例导引的增益 m=3
q(1)=0;%q, o, a 分别是追踪者的参数，初始时为 0。
o(1)=0;
a(1)=0;


for(k=2:600)%开始一个 for 循环，用于模拟追踪者追逐目标的过程，最多进行 600 步计算

% 目标位置随时间变化。目标在 XZ 平面上做匀速运动，轨迹沿着某个角度变化，pi/6 表示 30° 角，速度的两个分量分别是 X 和 Z 方向的速度。
ptr(:,k)=[25-0.42*cos(pi/6)*tt*k;5;10+0.42*sin(pi/6)*k*tt];

%计算上一时刻追踪者与目标之间的距离 r(k-1)，这是欧几里得距离公式。
r(k-1)=sqrt((ptr(1,k-1)-pmr(1,k-1))^2+(ptr(2,k-1)-pmr(2,k-1))^2+(ptr(3,k-1)-pmr(3,k-1))^2);

%计算上一时刻追踪者与目标之间的距离 r(k-1)，这是欧几里得距离公式。
c=sqrt((ptr(1,k)-pmr(1,k-1))^2+(ptr(2,k)-pmr(2,k-1))^2+(ptr(3,k)-pmr(3,k-1))^2);

%通过余弦定理计算角度 b 和 dq。这些角度用于判断追踪者和目标之间的相对方向变化。
b=acos((r(k-1)^2+st^2-c^2)/(2*r(k-1)*st));
dq=acos((r(k-1)^2-st^2+c^2)/(2*r(k-1)*c));

%检查角度 b 和 dq 是否为虚数。如果是虚数，意味着余弦定理解不出合理的角度，这里将虚数部分修正为极小值 0.0000001。
if abs(imag(b))>0
    b=0.0000001;
end 
if abs(imag(dq))>0
    dq=0.0000001;
end 

%更新追踪者的角度参数 q, o, 和 a，这些参数根据追踪者和目标的相对位置不断变化。
q(k)=q(k-1)+dq;
o(k)=o(k-1)+m*dq;
a(k)=o(k)-q(k);

%使用比例导引法的几何关系计算追踪者与目标的距离比值 c1 和 c2。
c1=r(k-1)*sin(b)/sin(a(k)+b);
c2=r(k-1)*sin(a(k))/sin(a(k)+b);

%计算新的距离 c3 和导引角度 dq，这些量用于进一步调整追踪者的运动。
c3=sqrt((c1-sm)^2+(c2-st)^2+2*(c1-sm)*(c2-st)*cos(a(k)+b));
dq=a(k)-acos(((c1-sm)^2+c3^2-(c2-st)^2)/(2*(c1-sm)*c3));

%再次检查是否出现虚数并修正角度 dq。
if abs(imag(dq))>0
dq=0.0000001;
end 

q(k)=q(k-1)+dq;
o(k)=o(k-1)+m*dq;
a(k)=o(k)-q(k);


c1=r(k-1)*sin(b)/sin(a(k)+b);
c2=r(k-1)*sin(a(k))/sin(a(k)+b);
c3=sqrt((c1-sm)^2+(c2-st)^2+2*(c1-sm)*(c2-st)*cos(a(k)+b));
dq=a(k)-acos(((c1-sm)^2+c3^2-(c2-st)^2)/(2*(c1-sm)*c3));
if abs(imag(dq))>0
dq=0.0000001;
end
q(k)=q(k-1)+dq;
o(k)=o(k-1)+m*dq;
a(k)=o(k)-q(k);
c1=r(k-1)*sin(b)/sin(a(k)+b);
c2=r(k-1)*sin(a(k))/sin(a(k)+b);
c3=sqrt((c1-sm)^2+(c2-st)^2+2*(c1-sm)*(c2-st)*cos(a(k)+b));

%计算追踪者前一时刻和当前时刻在三维空间中移动的中间位置（用于比例导引）。
x1(k)=ptr(1,k-1)+c2/st*(ptr(1,k)-ptr(1,k-1));
y1(k)=ptr(2,k-1)+c2/st*(ptr(2,k)-ptr(2,k-1));
z1(k)=ptr(3,k-1)+c2/st*(ptr(3,k)-ptr(3,k-1));

%根据中间位置，更新追踪者在当前时刻的实际位置。
x(k)=pmr(1,k-1)+sm/c1*(x1(k)-pmr(1,k-1));
y(k)=pmr(2,k-1)+sm/c1*(y1(k)-pmr(2,k-1));
z(k)=pmr(3,k-1)+sm/c1*(z1(k)-pmr(3,k-1));

%更新追踪者的全局位置 pmr(:,k) 和当前的距离 r(k)。
pmr(:,k)=[x(k);y(k);z(k)];
r(k)=sqrt((ptr(1,k)-pmr(1,k))^2+(ptr(2,k)-pmr(2,k))^2+(ptr(3,k)-pmr(3,k))^2);

%如果追踪者与目标的距离小于 0.06（达到某种碰撞条件），停止循环。
if r(k)<0.06;
break;
end;
end

% 输出追踪者与目标的遭遇时间。
sprintf('遭遇时间：%3.1f',0.1*k);

%使用 plot3 绘制追踪者和目标的三维轨迹，追踪者的轨迹为黑色，目标的轨迹根据之前的数据绘制。图形展示追踪者在空间中跟踪目标的过程。
figure(1);
plot3(pmr(1,1:k),pmr(2,1:k),pmr(3,1:k),'k',ptr(1,:),ptr(2,:),ptr(3,:));
axis([0 25 0 5 0 25]);

grid on