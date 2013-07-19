#include "mapserver.h"

void initLabelHitTests(labelObj *l, label_hittest *lh) {
  int i;
  lh->stylehits = msSmallCalloc(l->numstyles,sizeof(style_hittest));
}

void initClassHitTests(classObj *c, class_hittest *ch) {
  int i;
  ch->stylehits = msSmallCalloc(c->numstyles,sizeof(style_hittest));
  ch->labelhits = msSmallCalloc(c->numlabels,sizeof(label_hittest));
  for(i=0; i<c->numlabels; i++) {
    initLabelHitTests(c->labels[i],&ch->labelhits[i]);
  }
}
void initLayerHitTests(layerObj *l, layer_hittest *lh) {
  int i;
  lh->classhits = msSmallCalloc(l->numclasses,sizeof(class_hittest));
  for(i=0; i<l->numclasses; i++) {
    initClassHitTests(l->class[i],&lh->classhits[i]);
  }
}
void initMapHitTests(mapObj *map, map_hittest *mh) {
  int i;
  mh->layerhits = msSmallCalloc(map->numlayers,sizeof(layer_hittest));
  for(i=0; i<map->numlayers; i++) {
    initLayerHitTests(GET_LAYER(map,i),&mh->layerhits[i]);
  }
}

void freeLabelHitTests(labelObj *l, label_hittest *lh) {
  free(lh->stylehits);
}

void freeClassHitTests(classObj *c, class_hittest *ch) {
  int i;
  for(i=0; i<c->numlabels; i++) {
    freeLabelHitTests(c->labels[i],&ch->labelhits[i]);
  }
  free(ch->stylehits);
  free(ch->labelhits);
}
void freeLayerHitTests(layerObj *l, layer_hittest *lh) {
  int i;
  for(i=0; i<l->numclasses; i++) {
    freeClassHitTests(l->class[i],&lh->classhits[i]);
  }
  free(lh->classhits);
}
void freeMapHitTests(mapObj *map, map_hittest *mh) {
  int i;
  for(i=0; i<map->numlayers; i++) {
    freeLayerHitTests(GET_LAYER(map,i),&mh->layerhits[i]);
  }
  free(mh->layerhits);
}

int msHitTestShape(mapObj *map, layerObj *layer, shapeObj *shape, int drawmode, class_hittest *hittest) {
  int i,status;
  classObj *cp = layer->class[shape->classindex];
  if(MS_DRAW_FEATURES(drawmode)) {
    for(i=0;i<cp->numstyles;i++) {
      styleObj *sp = cp->styles[i];
      if(msScaleInBounds(map->scaledenom,cp->minscaledenom,cp->maxscaledenom)) {
        hittest->stylehits[i].status = 1;
      }
    }
  }
  if(MS_DRAW_LABELS(drawmode)) {
    for(i=0;i<cp->numlabels;i++) {
      labelObj *l = cp->labels[i];
      if(l->status == MS_ON) {
        int s;
        hittest->labelhits[i].status = 1;
        for(s=0; s<l->numstyles;s++) {
          hittest->labelhits[i].stylehits[s].status = 1;
        }
      }
    }
  }
  return MS_SUCCESS;
}

int msHitTestLayer(mapObj *map, layerObj *layer, layer_hittest *hittest) {
  int i,status;
  if(!msLayerIsVisible(map,layer)) {
    hittest->status = 0;
    return MS_SUCCESS;
  }
  if(layer->type == MS_LAYER_LINE || layer->type == MS_LAYER_POLYGON || layer->type == MS_LAYER_POINT || layer->type == MS_LAYER_ANNOTATION) {
    int maxfeatures=msLayerGetMaxFeaturesToDraw(layer, NULL);
    int annotate = msEvalContext(map, layer, layer->labelrequires);
    shapeObj shape;
    int nclasses,featuresdrawn = 0;
    int *classgroup;
    rectObj searchrect;
    int minfeaturesize=-1;
    if(map->scaledenom > 0) {
      if((layer->labelmaxscaledenom != -1) && (map->scaledenom >= layer->labelmaxscaledenom)) annotate = MS_FALSE;
      if((layer->labelminscaledenom != -1) && (map->scaledenom < layer->labelminscaledenom)) annotate = MS_FALSE;
    }

    status = msLayerOpen(layer);
    if(status != MS_SUCCESS) return MS_FAILURE;

    /* build item list */
    status = msLayerWhichItems(layer, MS_FALSE, NULL);

    if(status != MS_SUCCESS) {
      msLayerClose(layer);
      return MS_FAILURE;
    }

    /* identify target shapes */
    if(layer->transform == MS_TRUE) {
      searchrect = map->extent;
#ifdef USE_PROJ
      if((map->projection.numargs > 0) && (layer->projection.numargs > 0))
        msProjectRect(&map->projection, &layer->projection, &searchrect); /* project the searchrect to source coords */
#endif
    }
    else {
      searchrect.minx = searchrect.miny = 0;
      searchrect.maxx = map->width-1;
      searchrect.maxy = map->height-1;
    }

    status = msLayerWhichShapes(layer, searchrect, MS_FALSE);
    if(status == MS_DONE) { /* no overlap */
      msLayerClose(layer);
      hittest->status = 0;
      return MS_SUCCESS;
    } else if(status != MS_SUCCESS) {
      msLayerClose(layer);
      return MS_FAILURE;
    }

    /* step through the target shapes */
    msInitShape(&shape);

    nclasses = 0;
    classgroup = NULL;
    if(layer->classgroup && layer->numclasses > 0)
      classgroup = msAllocateValidClassGroups(layer, &nclasses);

    if(layer->minfeaturesize > 0)
      minfeaturesize = Pix2LayerGeoref(map, layer, layer->minfeaturesize);

    while((status = msLayerNextShape(layer, &shape)) == MS_SUCCESS) {
      int drawmode = MS_DRAWMODE_FEATURES;
      if(shape.type == MS_SHAPE_POLYGON) {
        msClipPolygonRect(&shape, map->extent);
      } else {
        msClipPolylineRect(&shape, map->extent);
      }
      if(shape.numlines == 0) {
        msFreeShape(&shape);
        continue;
      }
      /* Check if the shape size is ok to be drawn, we need to clip */
      if((shape.type == MS_SHAPE_LINE || shape.type == MS_SHAPE_POLYGON) && (minfeaturesize > 0)) {
        msTransformShape(&shape, map->extent, map->cellsize, NULL);
        msComputeBounds(&shape);
        if (msShapeCheckSize(&shape, minfeaturesize) == MS_FALSE) {
          msFreeShape(&shape);
          continue;
        }
      }

      shape.classindex = msShapeGetClass(layer, map, &shape, classgroup, nclasses);
      if((shape.classindex == -1) || (layer->class[shape.classindex]->status == MS_OFF)) {
        msFreeShape(&shape);
        continue;
      }
      hittest->classhits[shape.classindex].status = 1;
      hittest->status = 1;

      if(maxfeatures >=0 && featuresdrawn >= maxfeatures) {
        status = MS_DONE;
        break;
      }
      featuresdrawn++;

      if(annotate && layer->class[shape.classindex]->numlabels > 0) {
        msShapeGetAnnotation(layer, &shape);
        drawmode |= MS_DRAWMODE_LABELS;
      }

      status = msHitTestShape(map, layer, &shape, drawmode, &hittest->classhits[shape.classindex]); /* all styles  */
      msFreeShape(&shape);
    }

    if (classgroup)
      msFree(classgroup);

    if(status != MS_DONE) {
      msLayerClose(layer);
      return MS_FAILURE;
    }
    msLayerClose(layer);
    return MS_SUCCESS;

  } else {
    /* we don't hittest these layers, mark all as hit */
    int c;
    for(c=0; c<layer->numclasses; c++) {
      class_hittest *ch = hittest->classhits + c;
      int s,l;
      ch->status = 1;
      classObj *cp = layer->class[c];
      for(s=0;s<cp->numstyles;s++) {
        ch->stylehits[s].status = 1;
      }
      for(l=0;l<cp->numlabels;l++) {
        ch->labelhits[s].status = 1;
        for(s=0; s<cp->labels[l]->numstyles;s++) {
          ch->labelhits[l].stylehits[s].status = 1;
        }
      }
    }
    return MS_SUCCESS;
  }
}
int msHitTestMap(mapObj *map, map_hittest *hittest) {
  int i,status;
  for(i=0; i<map->numlayers; i++) {
    layerObj *lp = map->layers[i];
    status = msHitTestLayer(map,lp,&hittest->layerhits[i]);
    if(status != MS_SUCCESS) {
      return MS_FAILURE;
    }
  }
  return MS_SUCCESS;
}
