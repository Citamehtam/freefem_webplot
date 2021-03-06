//initial for 3D
var frustumSize;
var camera, scene, renderer, mesh, mesh_border, axes;
var controls;


//output btn for 3D PNG
$('#3dpng').click(function () {
    var canvas = document.querySelector("#new_plot > canvas");
    canvas.getContext("experimental-webgl", { preserveDrawingBuffer: true });
    var imgURI = canvas.toDataURL('image/png')
    blob = dataURItoBlob(imgURI)
    var w = window.open(URL.createObjectURL(blob), 'test.png');
});


function init() {
    max_x = basic_data.bounds[1][0] - basic_data.bounds[0][0];
    max_y = basic_data.bounds[1][1] - basic_data.bounds[0][1];
    max = Math.max(max_x, max_y);
    // max = basic_data.bounds[1][0] - basic_data.bounds[0][0];
    c = {
        x: (basic_data.bounds[1][0] + basic_data.bounds[0][0]) / 2,
        y: (basic_data.bounds[1][1] + basic_data.bounds[0][1]) / 2
    }
    const sc = 1000;
    frustumSize = sc + sc / 10;
    var aspect = 1;
    renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true, preserveDrawingBuffer: true });
    camera = new THREE.OrthographicCamera(frustumSize / -2 * max, frustumSize / 2 * max, frustumSize / 2 * max, frustumSize / -2 * max, 1, frustumSize *max);
    // camera = new THREE.PerspectiveCamera(frustumSize * max / 20, 1, 1, frustumSize * max);
    camera.position.set(0, 0, frustumSize);
    controls = new THREE.OrbitControls(camera, renderer.domElement);
    scene = new THREE.Scene();
    axes = new THREE.AxesHelper(frustumSize);
    axes.material.linewidth = 5;
    axes.position.set(-c.x * sc, -c.y * sc, 0); //move center
    
    var theatre = document.getElementById("new_plot")
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setSize(sc, (max_y / max_x)*sc);
    renderer.setClearAlpha(0);
    renderer.domElement.id = 'canvas_3d';

    theatre.appendChild(renderer.domElement);

}

function mydraw3d() {
    scene.remove(mesh);
    scene.remove(mesh_border);
    scene.remove(axes);
    const sc = 1000;
    const xc = sc;
    const yc = sc / (max_y / max_x);
    var zc = sc / (3 * (minmax_data[1].u - minmax_data[0].u));
    if (!isFinite(zc)) {
        zc = sc;
    }

    var geometry = new THREE.BufferGeometry();
    var vertices = [];
    var normals = [];
    var material = new THREE.ShaderMaterial({
        uniforms: {
            color1: {
                value: new THREE.Color(cprofile[0])
            },
            color2: {
                value: new THREE.Color(cprofile[1])
            },
            color3: {
                value: new THREE.Color(cprofile[2])
            },
            color4: {
                value: new THREE.Color(cprofile[3])
            },
            color5: {
                value: new THREE.Color(cprofile[4])
            },
            bboxMin: {
                value: {
                    'x': vertex_data[minmax_data[0].id].x * xc,
                    'y': vertex_data[minmax_data[0].id].y * yc,
                    'z': minmax_data[0].u * zc
                }
            },
            bboxMax: {
                value: {
                    'x': vertex_data[minmax_data[1].id].x * xc,
                    'y': vertex_data[minmax_data[1].id].x * yc,
                    'z': minmax_data[1].u * zc
                }
            }
        },
        vertexShader: `
                            uniform vec3 bboxMin;
                            uniform vec3 bboxMax;
                        
                            varying vec2 vUv;

                            void main() {
                                if ((bboxMax.z - bboxMin.z) > 0.0){
                                    vUv.y = (position.z - bboxMin.z) / (bboxMax.z - bboxMin.z);
                                }else{
                                    vUv.y = 0.0;
                                }
                                gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0);
                            }
                        `,
        fragmentShader: `
                            uniform vec3 color1;
                            uniform vec3 color2;
                            uniform vec3 color3;
                            uniform vec3 color4;
                            uniform vec3 color5;

                        
                            varying vec2 vUv;
                            
                            void main() {
                                if (vUv.y >= 0.75){
                                    gl_FragColor = vec4(mix(color4, color5, 4.0 * vUv.y - 3.0), 1.0);
                                }else if (vUv.y >= 0.5 && vUv.y < 0.75){
                                    gl_FragColor = vec4(mix(color3, color4, 4.0 * vUv.y - 2.0), 1.0);
                                }else if (vUv.y >= 0.25 && vUv.y < 0.5){
                                    gl_FragColor = vec4(mix(color2, color3, 4.0 * vUv.y - 1.0), 1.0);
                                }else{
                                    gl_FragColor = vec4(mix(color1, color2, 4.0 * vUv.y), 1.0);
                                }
                            }
                        `,
        wireframe: $("#wireframe").is(':checked'),
        wireframeLinewidth: 1 / 500 * sc,
    });
    material.needsUpdate = true


    mesh_data.forEach((e) => {
        var v0 = new THREE.Vector3((e[0].x - c.x) * xc, (e[0].y - c.y) * yc, e[0].u * zc);
        var v1 = new THREE.Vector3((e[1].x - c.x) * xc, (e[1].y - c.y) * yc, e[1].u * zc);
        var v2 = new THREE.Vector3((e[2].x - c.x) * xc, (e[2].y - c.y) * yc, e[2].u * zc);
        var vg = new THREE.Vector3();
        vg.add(v0).add(v1).add(v2).multiplyScalar(1 / 3);

        var triangle = new THREE.Triangle(v0, v1, v2);
        var normal = triangle.getNormal(vg);

        // var material = new THREE.MeshLambertMaterial({wireframe: true, wireframeLinewidth: 1, side: THREE.DoubleSide });
        vertices.push(triangle.a.x, triangle.a.y, triangle.a.z);
        vertices.push(triangle.b.x, triangle.b.y, triangle.b.z);
        vertices.push(triangle.c.x, triangle.c.y, triangle.c.z);
        normals.push(normal.x, normal.y, normal.z);
        normals.push(normal.x, normal.y, normal.z);
        normals.push(normal.x, normal.y, normal.z);

    })
 
    geometry.setAttribute('position', new THREE.Float32BufferAttribute(vertices, 3));
    geometry.setAttribute('normal', new THREE.Float32BufferAttribute(normals, 3));
    geometry.computeBoundingSphere();
    // var material = new THREE.MeshLambertMaterial({ wireframe: $("#wireframe").is(':checked'), wireframeLinewidth: 1 });
    mesh = new THREE.Mesh(geometry, material);
    scene.add(mesh);

    if (!$("#wireframe").is(':checked') && $("#mesh3d").is(':checked')) {
        var material = new THREE.LineBasicMaterial({
            color: 0x000000,
            linewidth: 1,
            linecap: 'round', //ignored by WebGLRenderer
            linejoin: 'round' //ignored by WebGLRenderer
        });
        var wireframe = new THREE.WireframeGeometry(geometry);
        mesh_border = new THREE.LineSegments(wireframe, material);
        mesh_border.material.depthTest = false;
        mesh_border.material.opacity = 0.25;
        mesh_border.material.transparent = true;
        scene.add(mesh_border);
    }

    if ($("#axes3d").is(':checked')){
        scene.add(axes);
    }

}

function animate() {
    if (!pause) {
        next();
        if (now_file >= total_file) {
            play();
        }
    }
    setTimeout(function () {
        requestAnimationFrame(animate);
    }, 50);

    renderer.render(scene, camera);
}


function view(x, y, z) {
    camera.position.set(x, y, z);
    controls.update();
}

