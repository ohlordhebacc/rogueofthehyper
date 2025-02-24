// show the fundamental domain for quotient spaces
// Copyright (C) 2018 Zeno and Tehora Rogue, see 'hyper.cpp' for details

#include "rogueviz.h"

namespace hr {

namespace fundamental {

transmatrix current_position, last_view;

void analyze_view_pre() {
  current_position = current_position * last_view * inverse(View);
  }

void analyze_view_post() {
  last_view = View;
  }

color_t color1, color2;

cell *starter;
map<cell*, set<int>> same;
map<cell*, shiftmatrix> gm;
int corners;
vector<hyperpoint> abs_cornerpos;
vector<cell*> cells;

vector<cellwalker> cornerlist;
map<cellwalker, int> corner_id;

bool is_connected(cellwalker cw) {
  return same[cw.at].count(cw.spin);
  }

void be_connected(cellwalker cw) {
  // transmatrix T = gm[cw.at];
  same[cw.at].insert(cw.spin);
  cw += wstep;
  same[cw.at].insert(cw.spin);
  /* printf("%s", display(T * C0)); 
  printf(" %s\n", display(gm[cw.at] * C0)); */
  // queueline(T * C0, gm[cw.at] * C0, 0xFF0000FF, 3);
  }

int funmode = 0;

bool single_edges = false;
bool fill_faces = true;

shiftpoint cwcorner(cellwalker cw) {
  shiftmatrix T = gm[cw.at];

  if(funmode == 2 && BITRUNCATED) {
    while(cw.at->type != S7) { 
      cw++; 
      T = T * currentmap->adj(cw.at, cw.spin);
      cw += wstep; 
      }
    return T * C0;
    }

  return gm[cw.at] * get_corner_position(cw.at, cw.spin+(cw.mirrored?0:1), 3);
  }

int group_count(cellwalker cw);

void auto_corners() {
  cellwalker cw;

  corners = 0;

  for(int k=0; k<isize(cells); k++) {
    cell *c = cells[k];
    for(int i=0; i<c->type; i++) {
      cellwalker cw0(c, i);
      if(single_edges) {
        if(!is_connected(cw0)) corners++, cw = cw0;
        }
      else {
        if(group_count(cw0) >= 3) corners++, cw = cw0;
        }
      }
    }

  if(!corners) return;

  cornerlist.clear();
  corner_id.clear();

  for(int ci=0; ci<corners; ci++) {
    corner_id[cw] = cornerlist.size();
    cornerlist.push_back(cw);

    while(true) {
      cw++;
      while(is_connected(cw)) {
        cw += wstep;
        cw++;
        }
      if(single_edges || group_count(cw) >= 3) break;
      }
    }
  auto corners0 = corners;
  corners = isize(cornerlist);
  cornerlist.push_back(cw);

  if(corners0 != corners) println(hlog, "corners=", tie(corners0, corners));

  abs_cornerpos.clear();
  for(auto c: cornerlist) {
    auto co = inverse_shift(gm[starter], cwcorner(c));
    if(isize(abs_cornerpos) && (hdist(co, abs_cornerpos[0]) < 1e-3 || hdist(co, abs_cornerpos.back()) < 1e-3)) continue;
    abs_cornerpos.push_back(co);
    }
  corners = isize(abs_cornerpos); abs_cornerpos.push_back(abs_cornerpos[0]);
  }

void find_corners() {

  abs_cornerpos.clear();

  auto build = [&] (int sides, ld a, ld a1, ld shift) {
    transmatrix T = Id;
    if(variation == eVariation::untruncated)
      T = T * rgpushxto0(get_corner_position(starter, 0, 3));
    for(int i=0; i<=sides; i++) {
      abs_cornerpos.push_back(T * xspinpush0(-i * TAU/sides + shift, (i&1) ? a : a1));
      }
    corners = sides;
    };

  if(funmode == 3) switch(geometry) {
    case gKleinQuartic: {
      ld a = edge_of_triangle_with_angles(90._deg, M_PI/14, M_PI*2/14);
      return build(14, a, a, 0);
      }
    case gSchmutzM2: {
      ld a = edge_of_triangle_with_angles(90._deg, M_PI/12, M_PI/6);
      ld a1 = edge_of_triangle_with_angles(45._deg, M_PI/12, 60._deg);
      return build(24, a1, a, 0);
      }
    case gSchmutzM3: {
      ld a = edge_of_triangle_with_angles(60._deg, M_PI/12, M_PI/12);
      ld a1 = edge_of_triangle_with_angles(M_PI/12, 60._deg, M_PI/12);
      return build(24, a, a1, 0);
      }
    case gBolza: {
      ld a = edge_of_triangle_with_angles(90._deg, M_PI/8, M_PI/8);
      return build(8, a, a, 22.5_deg);
      }
    case gBolza2: {
      ld a = edge_of_triangle_with_angles(90._deg, M_PI/8, M_PI/8);
      hyperpoint h1 = xspinpush0(0, a);
      hyperpoint h2 = xspinpush0(45._deg, a);
      hyperpoint hm = mid(h1, h2);
      ld a1 = hdist0(hm) * 2;
      return build(16, a, a1, 0);
      }
    default: break;
    }

  return auto_corners();
  }

transmatrix rel(cellwalker cw) {
  return currentmap->adj(cw.at, cw.spin);
  }

ld label_dist = .3;

shiftmatrix labelpos(shiftpoint h1, shiftpoint h2) {
  shiftpoint h = mid(h1, h2);
  shiftmatrix T = rgpushxto0(h);
  hyperpoint hx = inverse_shift(T, h2);
  ld alpha = atan2(-hx[1], hx[0]);
  return T * xspinpush(alpha + 90._deg, label_dist);
  }
 
ld widthfactor = 5;
ld label_scale = 1;

int group_count(cellwalker cw) {
  if(is_connected(cw)) return 0;
  auto cw1 = cw;
  int groups = 0;
  do {
    if(!is_connected(cw1)) groups++;
    cw1 = cw1 + wstep - 1;
    }
  while(cw1 != cw);
  return groups;
  }

map<unsigned, color_t> bucket_color;

int lq = 3, alpha = 32;

color_t *current_domain = nullptr;

cell *current_starter;

void compute_shape() {
  if(current_starter == starter) return;
  current_starter = starter;
  same.clear();
  gm.clear();
  
  gm[starter] = ggmatrix(starter);
  
  cells = {starter};
  
  int tree_edges = 0;
  int face_edges = 0;

  bool first_zebra_phase = geometry == gZebraQuotient && funmode == 3 && PURE;
  bool second_zebra_phase = false;

  again:

  for(int k=0; k<isize(cells); k++) {
    cell *c = cells[k];
    for(int i=0; i<c->type; i++) {
      cellwalker cw(c, i);
      cell *c2 = cw.cpeek();
      if(gm.count(c2)) continue;
      if(first_zebra_phase && (zebra40(c) % 4) != (zebra40(c2) % 4))
        continue;
      if(second_zebra_phase) {
        int j = 0;
        for(; j<c->type; j++) if(zebra40(c->move(j)) == 4 + zebra40(c) % 12) break;
        int v = gmod(i-j+2, 7); if(v < 5) continue;
        }
      gm[c2] = gm[c] * rel(cw);
      be_connected(cw);
      tree_edges++;
      cells.push_back(c2);
      }
    }

  if(first_zebra_phase) {
    first_zebra_phase = false;
    second_zebra_phase = true;
    goto again;
    }
  
  while(fill_faces) {
    int f = face_edges;
    for(int k=0; k<isize(cells); k++) {
      cell *c = cells[k];
      for(int i=0; i<c->type; i++) {
        cellwalker cw(c, i);
        if(group_count(cw) == 1) {
          face_edges++;
          be_connected(cw);
          }
        }
      }
    if(f == face_edges) break;
    }

  find_corners();
  }

void fundamental_marker() {
  current_domain = nullptr;
  if(!funmode || !quotient || GDIM == 3) return;
  compute_shape();
  if(!corners) return;

  set<unsigned> buckets_used;

  for(int i=0; i<corners; i++) curvepoint_pretty(abs_cornerpos[i], abs_cornerpos[i+1], lq);
  curvepoint_first();

  auto pos = current_position * last_view * inverse(View);

  vector<int> connections(corners, -1);
  vector<bool> mirrored(corners, false);
  map<unsigned, int> midedge_id;
  auto T = ggmatrix(starter);
  unsigned central_bucket = bucketer(unshift(T*C0));
  for(int i=0; i<corners; i++)
    midedge_id[bucketer(unshift(T*mid(abs_cornerpos[i], abs_cornerpos[i+1])))] = i;

  int next_connection_id = 0;

  dynamicval<ld> lw(vid.linewidth, vid.linewidth * widthfactor);

  for(auto c: cells)
  for(const shiftmatrix& V : hr::span_at(current_display->all_drawn_copies, c)) {
    auto V1 = V * inverse_shift(gm[c], gm[starter]);
    auto bu = bucketer(pos * unshift(V1*C0));
    if(buckets_used.count(bu)) continue;
    buckets_used.insert(bu);

    if(alpha && !bucket_color.count(bu)) {
      if(bucket_color.empty()) bucket_color[bu] = alpha;
      else bucket_color[bu] = (hrand(0x1000000) << 8) | alpha;
      }

    if(c == cwt.at && alpha && !current_domain) current_domain = &bucket_color[bu];
    queuecurve_reuse(V1, color1, alpha ? bucket_color[bu] : 0, PPR::LINE);

    if(bu != central_bucket) {
      for(int i=0; i<corners; i++) {
        unsigned val = bucketer(unshift(V1*mid(abs_cornerpos[i], abs_cornerpos[i+1])));
        auto p = at_or_null(midedge_id, val);
        if(p && connections[*p] == -1) {
          connections[*p] = connections[i] = next_connection_id++;
          mirrored[*p] = mirrored[i] = det(V1.T) * det(T.T) < 0;
          }
        }
      }
    }
  
  queuecurve(T, color2, 0, PPR::LINE);

  for(int ci=0; ci<corners; ci++) {
    int mc = (mirrored[ci] ? color1 : color2) >> 8;
    int id = connections[ci];
    if(id == -1) continue;
    queuestr(labelpos(T * abs_cornerpos[ci], T * abs_cornerpos[ci+1]), label_scale/cgi.scalefactor, its(id), mc);
    }
  }

void clear_data() {
  same.clear();
  gm.clear();
  bucket_color.clear();
  current_starter = nullptr;
  cornerlist.clear();
  corner_id.clear();
  abs_cornerpos.clear();
  cells.clear();
  }

void showMenu() {
  cmode = sm::SIDE | sm::MAYDARK;
  gamescreen();
  dialog::init(XLAT("display fundamental domains"), 0xFFFFFFFF, 150, 0);
  vector<string> mode_names = {"no display", "corners", "centers", "special"};
  dialog::addSelItem("mode", mode_names[funmode], 'm');
  dialog::add_action([] { funmode = (1 + funmode) % 4; clear_data(); });
  dialog::addSelItem("label distance", fts(label_dist), 'd');
  dialog::add_action([] {
    dialog::editNumber(label_dist, 0, 10, .1, 0.5, "label fistance", "label distance");
    });
  dialog::addSelItem("label scale", fts(label_scale), 's');
  dialog::add_action([] {
    dialog::editNumber(label_scale, 0, 10, .1, 0.5, "label scale", "label scale");
    });
  dialog::addSelItem("line width factor", fts(widthfactor), 'w');
  dialog::add_action([] {
    dialog::editNumber(widthfactor, 0, 5, .1, 1, "line width factor", "line width factor");
    });
  dialog::addColorItem("boundary of other domains", color1, 'o');
  dialog::add_action([] () {
    dialog::openColorDialog(color1, NULL);
    dialog::get_di().dialogflags |= sm::MAYDARK | sm::SIDE;
    });
  dialog::addColorItem("boundary of central domain", color2, 'p');
  dialog::add_action([] () {
    dialog::openColorDialog(color2, NULL);
    dialog::get_di().dialogflags |= sm::MAYDARK | sm::SIDE;
    });
  dialog::addSelItem("set the central tile to current position", its(celldistance(starter, cwt.at)), 's');
  dialog::add_action([] () {
    starter = cwt.at;
    });
  dialog::addBoolItem("remove internal lines", fill_faces, 'r');
  dialog::add_action([] { fill_faces = !fill_faces; clear_data(); });
  dialog::addBoolItem("all edges be single", single_edges, 'z');
  dialog::add_action([] { single_edges = !single_edges; clear_data(); });
  dialog::addSelItem("line quality", its(lq), 'w');
  dialog::add_action([] {
    dialog::editNumber(lq, 0, 5, 1, 3, "line quality", "line quality");
    });
  dialog::addSelItem("opacity of fill colors", its(alpha), 'a');
  dialog::add_action([] {
    dialog::editNumber(alpha, 0, 5, 16, 32, "shade alpha", "shade alpha");
    });
  if(alpha) {
    dialog::addItem("reshuffle all fill colors", 'r');
    dialog::add_action([] { bucket_color.clear(); });
    if(current_domain) {
      dialog::addColorItem("current central domain", *current_domain, 'y');
      dialog::add_action([] () {
        dialog::openColorDialog(*current_domain, NULL);
        dialog::get_di().dialogflags |= sm::MAYDARK | sm::SIDE;
        });
      }
    }

  dialog::addBack();
  dialog::display();
  }

void enable_fundamental() {
  start_game(); starter = cwt.at;
  rogueviz::rv_hook(hooks_frame, 100, fundamental_marker);
  rogueviz::rv_hook(hooks_clearmemory, 100, clear_data);
  rogueviz::rv_hook(hooks_o_key, 80, [] (o_funcs& v) { v.push_back(named_dialog("fundamental", showMenu)); });

  current_position = Id; last_view = View;
  rogueviz::rv_hook(hooks_preoptimize, 75, analyze_view_pre);
  rogueviz::rv_hook(hooks_postoptimize, 75, analyze_view_post);
  }

int readArgs() {
  using namespace arg;
           
  if(0) ;
  else if(argis("-fundamental0")) {
    enable_fundamental();
    }
  else if(argis("-fundamental")) {
    shift(); funmode = argi();
    shift(); color1 = arghex();
    shift(); color2 = arghex();
    shift_arg_formula(widthfactor);
    shift_arg_formula(label_scale);
    shift_arg_formula(label_dist);
    enable_fundamental();
    }
  else if(argis("-fundamental-more")) {
    shift(); single_edges = argi();
    shift(); fill_faces = argi();
    shift(); lq = argi();
    shift(); alpha = argi();
    }
  else return 1;
  return 0;
  }

auto fundamentalhook = addHook(hooks_args, 100, readArgs);
}
}
