#include "httplib.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#ifndef WITH_NO_INIT
#include "ff++.hpp"
#include "AFunction.hpp"
#include "AFunction_ext.hpp"
#endif
using namespace std;

using namespace Fem2D;
using namespace httplib;
Server svr;

namespace{
    const char DEFAULT_FETYPE[] = "P1";
}

std::string get_string(Stack stack, Expression e, const char *const DEFAULT)
{
    const size_t length = 128;
    char *const carg = new char[length];

    if (!e)
        strcpy(carg, DEFAULT);
    else
        strncpy(carg, GetAny<string *>((*e)(stack))->c_str(), length);

    return std::string(carg);
}


double myserver()
{

    svr.set_base_dir("./static");
    svr.Get("/", [](const Request &req, Response &res) {
        ifstream ifs("index.html");
        std::string hp_html((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        res.set_content(hp_html, "text/html");
    });
    cout << "server started" << endl;
    svr.listen("localhost", 1234);

    return 0.0;
}

// bool mywebplot(    Stack stack,
//                     KN<double> *const &pu,
//                     Fem2D::Mesh const * const &pTh)
// {
//     const Fem2D::Mesh &Th(*pTh);
//     KN<double> &u(*pu);
//     // cout << (size(u) )<<endl;

//     // ajax API part
//     svr.Get("/hi", [](const Request &req, Response &res) {
//         res.set_content("<svg height=\"250\" width=\"500\"><polygon points=\"220,10 300,210 170,250 123,234\" style=\"fill:lime;stroke:purple;stroke-width:1\" />Sorry, your browser does not support inline SVG.</svg>", "text/html");
//     });



//     svr.Get(R"(/hello/(\d+))", [](const Request &req, Response &res) {
//         auto numbers = req.matches[1];
//         res.set_content(numbers, "text/plain");
//     });

//     return true;
// }

class WEBPLOT_Op : public E_F0mps
{
  public:
    Expression eTh, ef;
    static const int n_name_param = 1;
    static basicAC_F0::name_and_type name_param[];
    Expression nargs[n_name_param];

    double arg(int i, Stack stack, double defvalue) const { return nargs[i] ? GetAny<double>((*nargs[i])(stack)) : defvalue; }
    long arg(int i, Stack stack, long defvalue) const { return nargs[i] ? GetAny<long>((*nargs[i])(stack)) : defvalue; }
    KN<double> *arg(int i, Stack stack, KN<double> *defvalue) const { return nargs[i] ? GetAny<KN<double> *>((*nargs[i])(stack)) : defvalue; }
    bool arg(int i, Stack stack, bool defvalue) const { return nargs[i] ? GetAny<bool>((*nargs[i])(stack)) : defvalue; }

  public:

    WEBPLOT_Op(const basicAC_F0 &args, Expression f, Expression th)
        : eTh(th), ef(f)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    AnyType operator()(Stack stack) const;
};

basicAC_F0::name_and_type WEBPLOT_Op::name_param[] =
    {
        // modify static const int n_name_param = ... in the above member
        {"fetype", &typeid(string *)}
        //{  "logscale",  &typeid(bool)} // not implemented
};

AnyType WEBPLOT_Op::operator()(Stack stack) const
{
    const std::string fetype = get_string(stack, nargs[0], DEFAULT_FETYPE);

    const Mesh *const pTh = GetAny<const Mesh *const>((*eTh)(stack));
    ffassert(pTh);
    const Fem2D::Mesh &Th(*pTh);
    const int nVertices = Th.nv;
    const int nTriangles = Th.nt;

    R2 Pmin, Pmax;
    Th.BoundingBox(Pmin, Pmax);

    const double &x0 = Pmin.x;
    const double &y0 = Pmin.y;

    const double &x1 = Pmax.x;
    const double &y1 = Pmax.y;

    const double unset = -1e300;
    KN<double> f_FE(Th.nv, unset);

    if (fetype != "P1")
    {
        std::cout << "plotPDF() : Unknown fetype : " << fetype << std::endl;
        std::cout << "plotPDF() : Interpolated as P1 (piecewise-linear)" << std::endl;
    }

    stringstream json;
    json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "[" << endl;
    for (int it = 0; it < Th.nt; it++)
    {
        for (int iv = 0; iv < 3; iv++)
        {

            int i = Th(it, iv);

            // if (f_FE[i] == unset)
            // {
                if (iv == 0)
                {
                    json << "  [ ";
                }
                else
                {
                    json << "    ";
                }
                MeshPointStack(stack)->setP(pTh, it, iv);
                f_FE[i] = GetAny<double>((*ef)(stack)); // Expression ef is atype<double>()
                json << "{\"index\":" << i << ",\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << ",\"u\":" << f_FE[i] << "}";

                if (iv != 2)
                {
                    json << "," << endl;
                }
                else
                {
                    json << "]" << endl;
                }
            // }


        }
        if (it != Th.nt - 1)
        {
            json << ",";
        }
        json << endl;
    }
    json << "]" << endl;
    string jsonstr;
    jsonstr = json.str();

    svr.Get("/mesh", [jsonstr](const Request &req, Response &res) {
        // generate element(triangle) JSON
        res.set_content(jsonstr, "application/json");
    });

    svr.Get("/vertex", [&, f_FE](const Request &req, Response &res) {
        // generate vertices JSON
        stringstream json;
        json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "[" << endl;
        for (int i = 0; i < Th.nv; i++)
        {
            json << "  {\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << ",\"u\":" << f_FE[i] << "}";
            if (i != Th.nv - 1)
            {
                json << ",";
            }
            json << endl;
        }
        json << "]" << endl;
        string jsonstr;
        jsonstr = json.str();
        res.set_content(jsonstr, "application/json");
    });

    svr.Get("/edge", [&, f_FE](const Request &req, Response &res) {
        // generate edge JSON
        stringstream json;
        json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "[" << endl;
        for (int i = 0; i < Th.neb; i++)
        {
            const int &v0 = Th(Th.bedges[i][0]);
            const int &v1 = Th(Th.bedges[i][1]);
            json << "  { \"label\": " << Th.be(i) << "," << endl;
            json << "    \"vertices\":[{\"x\":" << Th(v0).x << ",\"y\":" << Th(v0).y  << "}," << endl;
            json << "                {\"x\":" << Th(v1).x << ",\"y\":" << Th(v1).y << "}]" << endl;
            json << "  }";
            if (i != Th.neb - 1)
            {
                json << ",";
            }
            json << endl;
        }
        json << "]" << endl;
        string jsonstr;
        jsonstr = json.str();
        res.set_content(jsonstr, "application/json");
    });

    return true;
}




class WEBPLOT : public OneOperator
{
    const int argc;

  public:
    // WEBPLOT() : OneOperator(atype<long>(), atype<const Mesh *>(), atype<std::string *>()), argc(2) {}
    WEBPLOT(int) : OneOperator(atype<long>(), atype<double>(), atype<const Mesh *>()), argc(2) {}

    E_F0 *code(const basicAC_F0 &args) const
    {
        if (argc == 2)
            return new WEBPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]));
        else
            ffassert(0);
    }
};

static void init(){
    Global.Add("server", "(", new OneOperator0<double>(myserver));
    Global.Add("webplot", "(", new WEBPLOT(0));
}

LOADFUNC(init);
