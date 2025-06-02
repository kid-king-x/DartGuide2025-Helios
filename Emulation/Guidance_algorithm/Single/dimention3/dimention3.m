% ������ʼ��
g = 9.81; % �������ٶ� (m/s^2)
initial_velocity = 18; % ��ʼ�ٶ� (m/s)
initial_pitch_angle_1 = 30; % ��һ�������ǣ���ˮƽ��ļнǣ�(��)
initial_yaw_angle_1 = -7.3; % ��һ��ƫ���ǣ�ˮƽ������x��ļнǣ�(��)
initial_pitch_angle_2 = 30; % �ڶ��������ǣ���ˮƽ��ļнǣ�(��)
initial_yaw_angle_2 = -10; % �ڶ���ƫ���ǣ�ˮƽ������x��ļнǣ�(��)
time_of_flight = 5; % ����ʱ�� (s)�����ʱ����Ը�����Ҫ����
dt = 0.01; % ʱ�䲽�� (s)
time = 0:dt:time_of_flight; % ʱ������

% ����ʼ�ٶȷֽ�Ϊx��y��z����
initial_velocity_x_1 = initial_velocity * cosd(initial_pitch_angle_1) * cosd(initial_yaw_angle_1);
initial_velocity_y_1 = initial_velocity * cosd(initial_pitch_angle_1) * sind(initial_yaw_angle_1);
initial_velocity_z_1 = initial_velocity * sind(initial_pitch_angle_1);

initial_velocity_x_2 = initial_velocity * cosd(initial_pitch_angle_2) * cosd(initial_yaw_angle_2);
initial_velocity_y_2 = initial_velocity * cosd(initial_pitch_angle_2) * sind(initial_yaw_angle_2);
initial_velocity_z_2 = initial_velocity * sind(initial_pitch_angle_2);

% ��ʼ��λ�ú��ٶ�����
position_1 = zeros(3, length(time));
velocity_1 = zeros(3, length(time));
position_2 = zeros(3, length(time));
velocity_2 = zeros(3, length(time));
position_1(:, 1) = [0; 0; 0]; % ��ʼλ��
velocity_1(:, 1) = [initial_velocity_x_1; initial_velocity_y_1; initial_velocity_z_1]; % ��ʼ�ٶ�
position_2(:, 1) = [0; 0; 0]; % ��ʼλ��
velocity_2(:, 1) = [initial_velocity_x_2; initial_velocity_y_2; initial_velocity_z_2]; % ��ʼ�ٶ�

% ŷ����������λ�ú��ٶ�
for i = 1:length(time)-1
    % �����ٶȣ���z�������ܵ��������ٶ�Ӱ�죩
    velocity_1(:, i+1) = velocity_1(:, i) + [0; 0; -g] * dt;
    velocity_2(:, i+1) = velocity_2(:, i) + [0; 0; -g] * dt;
    
    % ����λ��
    position_1(:, i+1) = position_1(:, i) + velocity_1(:, i) * dt;
    position_2(:, i+1) = position_2(:, i) + velocity_2(:, i) * dt;
    
    % ������ڴ��أ���ֹͣ����
    if position_1(3, i+1) < 0 && position_2(3, i+1) < 0 
        position_1(3, i+1) = 0;
        velocity_1(:, i+1:end) = repmat([0; 0; 0], 1, length(time) - i);
        position_1(:, i+1:end) = repmat(position_1(:, i+1), 1, length(time) - i);
        position_2(3, i+1) = 0;
        velocity_2(:, i+1:end) = repmat([0; 0; 0], 1, length(time) - i);
        position_2(:, i+1:end) = repmat(position_2(:, i+1), 1, length(time) - i);
        break;
    end
end

% ���ƹ켣
figure;
plot3(position_1(1, :), position_1(2, :), position_1(3, :), 'r'); % ��ɫ�켣
hold on; % ���ֵ�ǰͼ�Σ�ʹ�ú�����ͼ���Ե�����ͬһ��ͼ��
plot3(position_2(1, :), position_2(2, :), position_2(3, :), 'b'); % ��ɫ�켣
xlabel('X (m)');
ylabel('Y (m)');
zlabel('Z (m)');
title('3D Dart Trajectory');
grid on;

% ����ͼ�δ��ڵı���
ax = gca; % ��ȡ��ǰ��������
ax.DataAspectRatio = [1, 1, 1]; % ȷ��x��y��z��ı�����ͬ

% ������ͼ��ʹԭ�������
view(30, 30); % ��λ��30�ȣ�����30��
