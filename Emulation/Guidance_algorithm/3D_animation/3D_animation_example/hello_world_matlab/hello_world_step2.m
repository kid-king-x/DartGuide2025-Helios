world = sim3d.World();
% 实例化一个名为圆柱体的参与者对象，可以使用任何角色名称。
actor = sim3d.Actor(ActorName='Cylinder');
% 为角色对象构建一个圆柱体形状并指定其大小。指定颜色。将actor对象添加到世界中。
createShape(actor,'cylinder',[0.5 0.5 .75]);
actor.Color = [1 0 1];
add(world,actor);
% 使用角色对象的平移、旋转和缩放属性来定位角色相对于世界原点的方向。
actor.Translation = [0 0 0];
actor.Rotation = [0 0 0];
actor.Scale = [1 1 1];
% 如果不创建视口，则设置默认视图，您可以使用键盘快捷键和鼠标控件在Simulation 3D Viewer窗口中导航。

% 对于这个例子，使用createViewport函数来创建一个viewport。
viewport = createViewport(world,Translation=[-4.5 0 1]);
sampletime = 0.02;
stoptime = 10;
run(world,sampletime,stoptime)
delete(world)

