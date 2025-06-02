world = sim3d.World();

% 使用createShape函数构建一个球形角色，并设置角色对象的属性。设置移动、重力和物理属性。
% 然后，构建一个平面角色。设置角色的preciseconcontacts属性，
% 使球角色与平面角色精确碰撞。
ball = sim3d.Actor();
createShape(ball,'sphere',[0.5 0.5 0.5]);
ball.Mobility = sim3d.utils.MobilityTypes.Movable;
ball.Gravity = true;
ball.Physics = true;
ball.Translation = [0 0 5];
ball.Color = [.77 0 .255];
ball.Shadows = false;
add(world,ball);

plane = sim3d.Actor();
createShape(plane,'plane', [5 5 0.1]);
plane.PreciseContacts = true;
add(world,plane);

% 如果不创建视口，则设置默认视图，您可以使用键盘快捷键和鼠标控件在Simulation 3D Viewer窗口中导航。
viewport = createViewport(world);


sampletime = 0.01;
stoptime = 10;
run(world,sampletime,stoptime)
delete(world)