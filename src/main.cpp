#include <crow/functional.h>
#include <crow/gates/udpgate.h>
#include <crow/tower.h>
#include <getopt.h>
#include <igris/trent/json.h>
#include <nos/print.h>
#include <rabbit/mesh/mesh.h>
#include <rabbit/mesh/primitives.h>
#include <rabbit/opengl/qtscene.h>
#include <rabbit/opengl/window.h>

bool CROW_ENABLED = true;
std::string MODEL_PATH = "model.json";

void print_help()
{
    printf("Usage: ctrans [OPTION]... ADDRESS\n"
           "\n"
           "Common option list:\n"
           "  -h, --help            print this page\n");
}

void parse_options(int argc, char *argv[])
{
    const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"model", required_argument, NULL, 'm'},
        {NULL, 0, NULL, 0}};

    int c;
    while ((c = getopt_long(argc, argv, "hm:", long_options, NULL)) != -1)
    {
        switch (c)
        {
        case 'h':
            print_help();
            exit(0);
        case 'm':
            MODEL_PATH = optarg;
            break;
        default:
            exit(1);
        }
    }
}

std::list<std::unique_ptr<rabbit::drawable_object>> drawables;

ralgo::pose3<float> trent_to_pose(const igris::trent &tr)
{
    auto tr_pose_lin = tr["lin"].as_list();
    auto tr_pose_ang = tr["ang"].as_list();
    auto lin = rabbit::vec3f{(float)tr_pose_lin[0].as_numer(),
                             (float)tr_pose_lin[1].as_numer(),
                             (float)tr_pose_lin[2].as_numer()};
    auto ang_vec = rabbit::vec3f{(float)tr_pose_ang[0].as_numer(),
                                 (float)tr_pose_ang[1].as_numer(),
                                 (float)tr_pose_ang[2].as_numer()};
    auto l = linalg::length(ang_vec);
    auto ang_quat = l == 0 ? rabbit::vec4f{0, 0, 0, 1}
                           : linalg::rotation_quat(ang_vec / l, l);
    auto pose = ralgo::pose3(ang_quat, lin);
    nos::println("pose: ", pose);
    nos::println("angvec: ", ang_vec);
    return pose;
}

int main(int argc, char *argv[])
{
    parse_options(argc, argv);

    auto window = rabbit::create_glfw_window();
    auto data = igris::json::parse_file(MODEL_PATH);

    if (CROW_ENABLED)
    {
        crow::create_udpgate(12);
    }

    rabbit::scene scene;
    auto view = scene.create_view();
    view->camera.set_camera(rabbit::vec3f{2.f, 10.f, 3}, {0, 0, 0});

    for (auto &tr_link : data["links"].as_list())
    {
        std::vector<rabbit::drawable_object *> link_drawables;
        nos::println("link: ", tr_link["name"].as_string());
        auto pose = trent_to_pose(tr_link["pose"]);

        rabbit::mesh<float> mesh;
        for (auto &tr_visual : tr_link["visual"].as_list())
        {
            nos::println("visual: ", tr_visual["geometry"].as_string());
            if (tr_visual["geometry"].as_string() == "cylinder")
            {
                auto radius = tr_visual["radius"].as_numer();
                auto height = tr_visual["height"].as_numer();
                mesh = rabbit::cylinder_mesh(radius, height, 10, 10);
            }
            else if (tr_visual["geometry"].as_string() == "box")
            {
                auto size = tr_visual["size"].as_list();
                mesh = rabbit::box_mesh(
                    size[0].as_numer(), size[1].as_numer(), size[2].as_numer());
            }
            else if (tr_visual["geometry"].as_string() == "sphere")
            {
                auto radius = tr_visual["radius"].as_numer();
                mesh = rabbit::sphere_mesh(radius, 10, 10);
            }
            auto color = tr_visual["color"].as_list();

            // mesh = rabbit::sphere_mesh(1, 10, 10);
            auto *drawable = new rabbit::mesh_drawable_object(mesh);
            drawable->set_color({(float)color[0].as_numer(),
                                 (float)color[1].as_numer(),
                                 (float)color[2].as_numer(),
                                 (float)color[3].as_numer()});
            drawable->set_pose(pose);
            scene.add_object(drawable);
            drawables.push_back(
                std::unique_ptr<rabbit::drawable_object>(drawable));
            link_drawables.push_back(drawable);
        }

        if (tr_link.have("crow_theme_json"))
        {
            auto crow_theme_json = tr_link["crow_theme_json"].as_string();
            crow::subscribe(crow_theme_json,
                            [link_drawables](igris::buffer data)
                            {
                                nos::println("crow_theme_json: ", data.size());
                                for (auto &drawable : link_drawables)
                                {
                                    nos::println("drawable: ", drawable);
                                    auto jdata =
                                        igris::json::parse(data.data());
                                    auto pose = trent_to_pose(jdata);
                                    drawable->set_pose(pose);
                                    nos::println(pose);
                                }
                            });
        }
    }

    crow::start_spin();
    while (!glfwWindowShouldClose(window))
    {
        // if (CROW_ENABLED)
        //     crow::onestep();
        scene.update();

        auto time = glfwGetTime();
        auto s = std::sin(time);
        auto c = std::cos(time);
        view->camera.set_camera(rabbit::vec3f{s * 10, c * 10, 3}, {0, 0, 0});

        glfwPollEvents();
        glfwSwapBuffers(window);
    }
}