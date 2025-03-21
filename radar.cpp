#include "hyper.h"
namespace hr {

#if MAXMDIM >= 4
pair<bool, hyperpoint> makeradar(shiftpoint h, bool distant) {

  hyperpoint h1;

  if(embedded_plane) h1 = current_display->radar_transform * unshift(h);
  else if(sol && nisot::geodesic_movement) {
    hyperpoint h1 = inverse_exp(h, pQUICK);
    ld r = hypot_d(3, h1);
    if(r < 1) h1 = h1 * (atanh(r) / r);
    else return {false, h1};
    }
  else if(mproduct) h1 = product::inverse_exp(unshift(h));
  else if(sl2) h1 = slr::get_inverse_exp(h);
  else h1 = unshift(h);

  if(nisot::local_perspective_used && !embedded_plane) {
    h1 = NLP * h1;
    }
  
  if(WDIM == 3) {
    ld d = hdist0(h);
    if(distant) {
      h1 = h1 / hypot_d(3, h1);
      }
    else {
      if(d >= vid.radarrange) return {false, h1};
      if(d) h1 = h1 * (d / vid.radarrange / hypot_d(3, h1));
      }
    }
  else {
    h1 = cgi.emb->actual_to_base(h1);
    h1 = current_display->radar_transform_post * h1;
    if(mhyperbolic) {
      h1[LDIM] = h1[2]; if(!gproduct) h1[2] = 0;
      for(int a=0; a<LDIM; a++) h1[a] = h1[a] / (1 + h1[LDIM]);
      h1[LDIM] *= 2;
      }
    if(meuclid) {
      ld d = hypot_d(2, h1);
      if(d > vid.radarrange) return {false, h1};
      if(d) h1 = h1 / (vid.radarrange + cgi.scalefactor/4);
      }
    /* no change for sphere! */
    }
  if(invalid_point(h1)) return {false, h1};
  return {true, h1};
  }

EX void addradar(const shiftmatrix& V, char ch, color_t col, color_t outline, bool distant IS(false)) {
  shiftpoint h = V * tile_center();
  auto hp = makeradar(h, distant);
  if(hp.first)
    current_display->radarpoints.emplace_back(radarpoint{hp.second, ch, col, outline});
  }

EX void addradar(const shiftpoint h1, const shiftpoint h2, color_t col) {
  auto hp1 = makeradar(h1, false);
  auto hp2 = makeradar(h2, false);
  if(hp1.first && hp2.first)
    current_display->radarlines.emplace_back(radarline{hp1.second, hp2.second, col});
  }

void celldrawer::drawcell_in_radar() {
  #if CAP_SHMUP
  if(shmup::on) {
    pair<shmup::mit, shmup::mit> p = 
      shmup::monstersAt.equal_range(c);
    for(shmup::mit it = p.first; it != p.second; it++) {
      shmup::monster* m = it->second;
      addradar(V*m->at, minf[m->type].glyph, minf[m->type].color, 0xFF0000FF);
      }
    }
  #endif
  if(c->monst) 
    addradar(V, minf[c->monst].glyph, minf[c->monst].color, isFriendly(c->monst) ? 0x00FF00FF : 0xFF0000FF);
  else if(c->item && !itemHiddenFromSight(c))
    addradar(V, iinf[c->item].glyph, iinf[c->item].color, kind_outline(c->item));
  }

void celldrawer::radar_grid() {
  for(int t=0; t<c->type; t++)
    if(c->move(t) && (c->move(t) < c || fake::split()))
      addradar(V*get_corner_position(c, t%c->type), V*get_corner_position(c, (t+1)%c->type), gridcolor(c, c->move(t)));
  }
#endif

EX void draw_radar(bool cornermode) {
#if MAXMDIM >= 4
  if(subscreens::split([=] () { calcparam(); draw_radar(false); })) return;
  if(dual::split([] { dual::in_subscreen([] { calcparam(); draw_radar(false); }); })) return;
  bool d3 = WDIM == 3;
  int ldim = LDIM;
  bool hyp = mhyperbolic;
  bool sph = msphere;
  bool scompass = nonisotropic && !mhybrid && !embedded_plane;

  dynamicval<eGeometry> g(geometry, gEuclid);
  dynamicval<eModel> pm(pmodel, mdDisk);
  dynamicval<bool> ga(vid.always3, false);
  dynamicval<geometryinfo1> gi(ginf[gEuclid].g, giEuclid2);
  initquickqueue();
  int rad = vid.radarsize;
  int divby = 1;
  if(dual::state) divby *= 2;
  if(subscreens::in) divby *= 2;
  rad /= divby;
  auto& cd = current_display;
  
  ld cx = dual::state ? (dual::currently_loaded ? vid.xres/2+rad+2 : vid.xres/2-rad-2) :
          subscreens::in ? cd->xtop + cd->xsize - rad - 2 :
          cornermode ? rad+2+vid.fsize : vid.xres-rad-2-vid.fsize;
  ld cy = subscreens::in ? cd->ytop + cd->ysize - rad - 2 - vid.fsize :
          vid.yres-rad-2 - vid.fsize;
  
  auto ASP = atscreenpos(0, 0);

  for(int i=0; i<=360; i++)
    curvepoint(eupoint(cx-cos(i * degree)*rad, cy-sin(i*degree)*rad));
  queuecurve(ASP, 0xFFFFFFFF, 0x000000FF, PPR::ZERO);      

  ld alpha = 15._deg;
  ld co = cos(alpha);
  ld si = sin(alpha);
  
  if(sph && !d3) {
    for(int i=0; i<=360; i++)
      curvepoint(eupoint(cx-cos(i * degree)*rad, cy-sin(i*degree)*rad*si));
    queuecurve(ASP, 0, 0x200000FF, PPR::ZERO);
    }

  if(d3) {
    for(int i=0; i<=360; i++)
      curvepoint(eupoint(cx-cos(i * degree)*rad, cy-sin(i*degree)*rad*si));
    queuecurve(ASP, 0xFF0000FF, 0x200000FF, PPR::ZERO);
  
    curvepoint(eupoint(cx-sin(vid.fov*degree/2)*rad, cy-sin(vid.fov*degree/2)*rad*si));
    curvepoint(eupoint(cx, cy));
    curvepoint(eupoint(cx+sin(vid.fov*degree/2)*rad, cy-sin(vid.fov*degree/2)*rad*si));
    queuecurve(ASP, 0xFF8000FF, 0, PPR::ZERO);
    }
  
  if(d3) for(auto& r: cd->radarpoints) {
    queueline(ASP*eupoint(cx+rad * r.h[0], cy - rad * r.h[2] * si + rad * r.h[1] * co), ASP*eupoint(cx+rad*r.h[0], cy - rad*r.h[2] * si), r.line, -1);
    }
  
  if(scompass) {
    auto compassdir = [&] (char dirname, hyperpoint h) {
      h = NLP * h * .8;
      queueline(ASP*eupoint(cx+rad * h[0], cy - rad * h[2] * si + rad * h[1] * co), ASP*eupoint(cx+rad*h[0], cy - rad*h[2] * si), 0xA0401040, -1);
      displaychr(int(cx+rad * h[0]), int(cy - rad * h[2] * si + rad * h[1] * co), 0, 8 * mapfontscale / 100, dirname, 0xA04010);
      };
    compassdir('E', point3(+1, 0, 0));
    compassdir('N', point3(0, +1, 0));
    compassdir('W', point3(-1, 0, 0));
    compassdir('S', point3(0, -1, 0));
    compassdir('U', point3(0,  0,+1));
    compassdir('D', point3(0,  0,-1));
    }

  ld f = cgi.scalefactor;
  if(cgi.emb->is_euc_in_hyp()) f /= exp(vid.depth);

  auto locate = [&] (hyperpoint h) {
    if(sph)
      return point3(cx + (rad-10) * h[0], cy + (rad-10) * h[2] * si + (rad-10) * h[1] * co, +h[1] * si > h[2] * co ? 8 : 16);
    else if(hyp) 
      return point3(cx + rad * h[0], cy + rad * h[1], 1/(1+h[ldim]) * cgi.scalefactor * current_display->radius / (inHighQual ? 10 : 6));
    else
      return point3(cx + rad * h[0], cy + rad * h[1], rad * f / (vid.radarrange + f/4) * 0.8);
    };
  
  for(auto& r: cd->radarlines) {
    hyperpoint h1 = locate(r.h1);
    hyperpoint h2 = locate(r.h2);
    queueline(ASP*eupoint(h1[0], h1[1]), ASP*eupoint(h2[0], h2[1]), r.line, -1);
    }

  quickqueue();
  glflush();
  
  for(auto& r: cd->radarpoints) {
    if(d3) displaychr(int(cx + rad * r.h[0]), int(cy - rad * r.h[2] * si + rad * r.h[1] * co), 0, 8 * mapfontscale / 100, r.glyph, r.color);
    else {
      hyperpoint h = locate(r.h);
      displaychr(int(h[0]), int(h[1]), 0, int(h[2]) * mapfontscale / divby / 100, r.glyph, r.color);
      }
    }
#endif
  }

#if MAXMDIM < 4
EX void addradar(const shiftmatrix& V, char ch, color_t col, color_t outline) { }
  void drawcell_in_radar();

void celldrawer::drawcell_in_radar() {}
void celldrawer::radar_grid() {}
#endif
}
