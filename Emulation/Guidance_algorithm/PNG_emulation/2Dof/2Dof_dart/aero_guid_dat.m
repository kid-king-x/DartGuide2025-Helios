% AERO_GUID_DAT	Initialization file for missile guidance model 导弹制导模型初始化文件
%
% See also: AERO_SANIM and Simulink model 'aero_guidance'

%   J.Hodgson
%   Copyright 1990-2008 The MathWorks, Inc.

%==================================================================
% Useful Constants 通用常量
%==================================================================

d2r     = pi/180;                 % Conversion Deg to Rad
g       = 9.81;                   % Gravity [m/s/s]
m2ft    = 3.28084;                % metre to feet 米到英尺的转换
Kg2slug = 0.0685218;              % Kg to slug

%==================================================================
% Atmospheric Constants 大气常量
%==================================================================

T0      = 288.16;                 % Temp. at Sea Level [K]
rho0    = 1.225;                  % Density [Kg/m^3]
L       = 0.0065;                 % Lapse Rate [K/m]
R       = 287.26;                 % Gas Constant J/Kg/K
gam     = 1.403;                  % Ratio of Specific Heats
P0      = 101325.0;               % Pressure at Sea Level [N/m^2]
h_trop  = 11000.0;                % Height of Troposphere [m]

%==================================================================
% Missile Configuration
%==================================================================
S_ref   = 0.44/m2ft^2;            % Reference area [m^2]
d_ref   = 0.75/m2ft;              % Reference length [m]
Iyy     = 182.5/(Kg2slug*m2ft^2); % Inertia
mass    = 13.98/Kg2slug;          % Mass [Kg]
Thrust  = 10e3;                   % Thrust [N]

%==================================================================
% Missile Aerodynamics
%==================================================================
Mach_vec  = 2:0.5:4;              % Reference Mach Numbers 参考马赫数
alpha_vec = (-20:1:20)'*d2r;      % Reference Incidence Values [rad] 参考入射值
[M,al]=meshgrid(Mach_vec,alpha_vec/d2r);

% Axial Force Coefficient
Cx_alpha = -0.3*ones(length(alpha_vec),length(Mach_vec));

% Normal Force Coefficient
an    =	 0.000103;                % [Deg^-3]
bn    =	-0.009450; 		  % [Deg^-2]
cn    = -0.169600;		  % [Deg^-1]
Cz_alpha = an*al.^3 + bn*al.*abs(al) + cn*(2-M/3).*al;
Cz_el = -0.034000/d2r;	

% Moment Coefficient
am       = 0.000215;              % [Deg^-3]
bm       =-0.019500;              % [Deg^-2]
cm       = 0.051000;              % [Deg^-1]
Cm_alpha = am*al.^3 + bm*al.*abs(al) - cm*(7-8*M/3).*al;
Cm_el    = -0.206000/d2r;
Cm_q     = -1.719;

%==================================================================
% Define Initial Conditions
%==================================================================
x_ini      = 0;		        % Initial downrange position [m] 初始下射程位置
h_ini      = 10000/m2ft;        % Initial altitude [m] 初始高度
v_ini      = 3*328;		% Initial velocity [m/s] 初始总速度
alpha_ini  = -10*d2r;		% Initial incidence [rad] 初始入射角
theta_ini  = 0*d2r;		% Initial Body Attitude [rad] 初始身体姿态
q_ini      = 0*d2r;		% Initial pitch rate [rad/sec] 初始的pitch角速度

%==================================================================
% Define Target 
%==================================================================
pos_tgt   = [4500+x_ini -h_ini]; % Initial Target position [m] 初始的目标位置
v_tgt     = 328;		% Target Velocity [m/s]
a_tgt = 0;

%==================================================================
% Missile Actuators
%==================================================================
wn_fin      = 150.0;            % Actuator Bandwidth [rad/sec]驱动器带宽
z_fin 	    = 0.7;              % Actuator Damping驱动器阻尼
fin_act_0   = 0.0;              % Initial Fin Angle [rad]
fin_max     =  30.0*d2r;        % Max Fin Deflection [rad] 
fin_min	    = -30.0*d2r;        % Min Fin Deflection [rad]
fin_maxrate = 500*d2r;          % Max Fin Rate [rad/sec]

%==================================================================
% Sensors
%==================================================================
l_acc       = 0.5;     % Position of accelerometer ahead of c.g [m]加速度计在c.g.前方的位置

%==================================================================
% Define Homing Head Dynamics
%==================================================================
wn_hom	    = 7.0;		% Estimator Bandwidth [rad/sec]
tors  	    = 0.02;		% Tracking Loop Time Constant [sec]跟踪环路时间常数
max_gimbal  = 35*d2r;		% Maximum Gimbal Angle [rad]
min_gimbal  = -35*d2r;		% Minimum Gimbal Angle [rad]
wgyro       = 100*2*pi;		% Rate gyro bandwidth	[rad/sec]
Ks          = wgyro/5;		% Rate Loop Bandwidth [rad/sec]
K_r         = -0.02;		% Radome Aberration
Beamwidth   = 10*d2r;		% Radar Beam Width [rad]

%==================================================================
% Load Autopilot Gains
%==================================================================
load aero_guid_autop	        % Gain Scheduled autopilot gains
max_acc = 40*9.81;              % Maximum demanded acceleration




