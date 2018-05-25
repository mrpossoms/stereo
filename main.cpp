#include <seen/seen.hpp>

using namespace seen;

union rgba_t {
	struct { uint8_t r, g, b, a; };
	uint8_t v[4];
};

struct patch_t {
	int x, y;
	int w, h;
};

const float IPD = 0.1;

inline float patch_difference(rgba_t* eye[2], patch_t patch[2], int width, int patch_size)
{
	int area = patch_size * patch_size;
	int deltas[area * 3];
	for(int i = area; i--;)
	{
		int row = i / patch_size;
		int col = i % patch_size;
		rgba_t l = eye[0][((patch[0].y + row) * width) + patch[0].x + col];
		rgba_t r = eye[1][((patch[1].y + row) * width) + patch[1].x + col];

		deltas[i * 3 + 0] = l.r - r.r;
		deltas[i * 3 + 1] = l.g - r.g;
		deltas[i * 3 + 2] = l.b - r.b;
	}

	int sum_sqr = 0;
	for (int i = area * 3; i--;)
	{
		sum_sqr += deltas[i] * deltas[i];
	}

	return sum_sqr;
}


int clamp(int i, int low, int high)
{
	if (i > high) return high;
	if (i < low) return low;
	return i;
}


int main (int argc, char const * argv[])
{

	RendererGL renderer("./data", "Stereo Vision Demo", 640, 480);
	Camera camera(M_PI / 2, renderer.width, renderer.height);

	int width, height, depth;
	rgba_t* eye_buffer[2];
	std::string names[] = { "/l.png", "/r.png" };

	Quat pointcloud_ori;

	camera.position(0, -0.25, -1);

	renderer.mouse_moved = [&](double x, double y, double dx, double dy) {
		Quat pitch, yaw;
		// quat_from_axis_angle(pitch.v, 1, 0, 0, dy * 0.01f);
		quat_from_axis_angle(yaw.v, 0, 1, 0, dx * 0.01f);
		pointcloud_ori = pointcloud_ori * pitch * yaw;
	};

	// load the left and right eye frames
	for (int i = 2; i--;)
	TextureFactory::load_texture_buffer(
		std::string(argv[1]) + names[i],
		(void**)&eye_buffer[i],
		width, height, depth
	);

	time_t start = time(NULL);

	int win_size = 5;
	int dm_w = width - win_size + 1, dm_h = height / win_size;
	float depth_map[dm_w * dm_h];
	rgba_t depth_map_colors[dm_w * dm_h];

	while(time(NULL) == start) usleep(1);
	start = time(NULL);

	int cycles = 0;
	while(time(NULL) == start)
	{
		for (int row = dm_h; row--;)
		for (int j = dm_w; j--;)
		{
			patch_t patches[] = {
				{ j, row * win_size }, { j, row * win_size }
			};
			int bi = j;
			float best_diff = patch_difference(eye_buffer, patches, width, win_size);

			int search_range = win_size * width / 32;
			int mr_low = clamp(j - search_range, 0, width - 1);
			int mr_high = clamp(j + search_range, 0, width - 1);
			// int w_low = clamp(row, 0, height - 1);
			// int w_high = clamp(row+1, 0, height - 1);

			// for (int w = w_low; w < w_high; w++)
			for (int i = mr_low; i < mr_high; i++)
			{
				patches[1].x = i;
				// patches[1].y = row * win_size + w;

				float diff = patch_difference(eye_buffer, patches, width, win_size);
				if (diff < best_diff)
				{
					best_diff = diff;
					bi = i;
				}
			}

			float patch_dist = fabs(bi - j) / (float)width;
			// float disp = patch_dist * 10;
			// disp += 1;
			// depth_map[row * dm_w + j] = disp / (disp * disp);
			depth_map[row * dm_w + j] = log((patch_dist / IPD)) + 1;
			depth_map_colors[row * dm_w + j] = eye_buffer[0][(patches[0].y * width) + patches[0].x];
			// std::cerr <<  "(" << j << " | " << bi << ") " << best_diff << std::endl;
		}
		cycles++;
	}

	std::cerr << cycles << " fps" << std::endl;


	CustomPass pointcloud_pass([&](){
		glMatrixMode(GL_PROJECTION);
		gl_get_error();
		glLoadMatrixf((float*)camera._projection.v);
		gl_get_error();

		{
			mat4x4 world;
			mat4x4_from_quat(world, pointcloud_ori.v);

			glMatrixMode(GL_MODELVIEW);
			gl_get_error();
			glLoadMatrixf((float*)camera._view.v);
			glPushMatrix();
			glMultMatrixf((float*)world);
			gl_get_error();
		}

		glBegin(GL_POINTS);
		for (int row = 0; row < dm_h; row += 2)
		{
			for (int j = 0; j < dm_w; ++j)
			{
				float z = depth_map[(row * dm_w) + j];
				rgba_t c = depth_map_colors[(row * dm_w) + j];
				float x = j / (float)dm_w, y = row / (float)dm_h;

				// if (z == 0 || z == 1) continue;

				float inv_z = 1;// - z;
				glColor3ub(c.r * inv_z, c.g * inv_z, c.b * inv_z);
				glVertex3f(-(x - 0.5), -(y - 0.5), z - 0.5);
			}
		}
		glEnd();
		glPopMatrix();
	}, NULL);

	glPointSize(5);

	ListScene scene;
	scene.drawables().push_back(&pointcloud_pass);

	while(renderer.is_running())
	{
		renderer.prepare();
		renderer.draw(NULL, &scene);
	}

	return 0;
}
