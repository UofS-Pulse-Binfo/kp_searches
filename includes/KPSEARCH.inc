<?php
/**
 * Provides a search for all chado stock-based Tripal Content regardless of type.
 *
 */
class KPSEARCH extends ChadoCustomSearch {

  /**
   * Adds a KnowPulse-specific crop selector to the form.
   */
  public function addCropFormElement(&$form, &$form_state) {

    // Container.
    $form['crops'] = array(
      '#type' => 'markup',
      '#prefix' => '<div class="crop-element form-item"><label>Crop</label>',
      '#suffix' => '</div>',
      '#weight' => -9,
    );

    // Crop options.
    $options = unserialize(variable_get('kp_searches_crop_options', 'a:0:{}'));

    // Determine the current crop, if one is selected.
    $q = drupal_get_query_parameters();
    if (isset($form_state['values'])) {
      $genus_default = $form_state['values']['genus'];
    }
    elseif (is_array($q) AND isset($q['genus'])) {
      $genus_default = $q['genus'];
    }
    else {
      $class = get_called_class();
      $details = $class::$info['filters']['genus'];
      $genus_default = (isset($details['default'])) ? $details['default'] : NULL;
    }

    // Now add the form elements for each crop.
    foreach ($options as $crop_key => $crop) {

      // Determine whether this is the active genus or not.
      $class = 'active';
      if ($genus_default !== NULL AND $genus_default != $crop_key) {
        $class = 'inactive';
      }

      // Format the title as a link to the search.
      $search_url = $this::$menu['path'];
      $search_q = $q;
      $search_q['genus'] = $crop['genus'];
      //$search_q['species'] = $crop['crop-species'];
      $crop_link = l($crop['title'], $search_url, [
        'query' => $search_q,
        'attributes' => ['class' => [$class]]
      ]);

      // Format the image as a link to the search.
      $image = theme('image', [
        'path' => url($crop['image']),
        'width' => 75,
        'height' => 75,
        'alt' => $crop['title'],
      ]);
      $crop_img_link = l($image, $search_url, [
        'html' => TRUE,
        'query' => $search_q,
        'attributes' => ['class' => [$class]]
      ]);

      // Contain both the image and title links in a container
      // and add to the form.
      $form['crops'][$crop_key] = [
        '#type' => 'markup',
        '#markup' => '<span class="crop">'
           . '<div class="crop-img">' . $crop_img_link . '</div>'
           . '<div class="crop-title">' . $crop_link . '</div>'
           . '</span>',
      ];
    }
  }

  /**
   * Adds a Scientific name field with the genus set based on crop.
   * @todo make this AJAX-driven.
   */
  public function addSpeciesFormElement(&$form, &$form_state) {
    $class = get_called_class();

    $description = $class::$info['filters']['genus']['help'];
    $form['scientific_name'] = array(
      '#type' => 'markup',
      '#prefix' => '<div id="scientific-nameform-item" class="scientific-name-element form-item"><label>Scientific Name</label>',
      '#suffix' => '<div class="description">' . $description . '</div></div>',
      '#weight' => -8,
    );

    $options = unserialize(variable_get('kp_searches_crop_options', 'a:0:{}'));
    $genus_options = [];
    foreach ($options as $genus => $crop_details) {
      $genus_options[ $genus ] = $genus;
    }

    $q = drupal_get_query_parameters();
    if (isset($form_state['values'])
      AND isset($options[ $form_state['values']['genus'] ])) {
      $species = $options[ $form_state['values']['genus'] ]['species'];
      $genus_default = $form_state['values']['genus'];
      $species_default = $form_state['values']['species'];
    }
    elseif (isset($q['genus']) AND isset($options[ $q['genus'] ])) {
      $species = $options[ $q['genus'] ]['species'];
      $genus_default = $q['genus'];
      $species_default = $q['species'];
    }
    else {
      $details = $class::$info['filters']['genus'];
      $genus_default = (isset($details['default'])) ? $details['default'] : NULL;

      $species = [];
      $details = $class::$info['filters']['species'];
      $species_default = (isset($details['default'])) ? $details['default'] : NULL;
    }

    $form['scientific_name']['genus'] = $form['genus'];
    $form['scientific_name']['genus']['#required'] = TRUE;
    $form['scientific_name']['genus']['#type'] = 'select';
    $form['scientific_name']['genus']['#options'] = $genus_options;
    $form['scientific_name']['genus']['#empty_option'] = '- Select Genus -';
    $form['scientific_name']['genus']['#default_value'] = $genus_default;
    $form['scientific_name']['genus']['#attributes'] = ['onchange' => 'this.form.submit();'];
    unset($form['genus']);


    $form['scientific_name']['species'] = $form['species'];
    $form['scientific_name']['species']['#type'] = 'select';
    $form['scientific_name']['species']['#options'] = $species;
    $form['scientific_name']['species']['#empty_option'] = '- Select Species -';
    $form['scientific_name']['species']['#default_value'] = $species_default;
    unset($form['species']);
  }

  /**
   * Add field to allow filter based on germplasm collection
   * or population information.
   * 
   * Source: stockcollection, stockcollection_stock and stock tables.
   *
   * @param $form
   *   Drupal form object.
   * @param $form_state
   *   Drupal form state object.
   */
  public function addCollectionFormElement(&$form, &$form_state) {
    $class = get_called_class();
     
    $default_text = '- Select -';
    // Set default to all collection.
    $collection_options = [0 => $default_text]; 
    // Default to select (option 0 above array, of the field).
    $default_collection = 0; 

    // Get collections in chado.stockcollections.
    $sql_collection = "SELECT stockcollection_id AS id, name 
      FROM chado.stockcollection ORDER BY name ASC";
    
    $collections = chado_query($sql_collection);    
    if ($collections->rowCount() > 0) {
      foreach($collections as $collection) {
        // Collection id number - Collection Name: Field will return
        // collection id number, NOT THE NAME.
        $collection_options[ $collection->id ] = $collection->name;
      }
    }
    
    $q = drupal_get_query_parameters();
    
    // Refine collection options to only collection names
    // where a registered germplasm has the genus of the selected genus.
    if (isset($q) && is_array($q) && $collection_options) {
      if (isset($q['genus']) && !empty($q['genus'])) {
        $genus = $q['genus'];
        
        // A single stock matching the genus (from collection) would be enough
        // to satisfy that collection has a germplasm with the genus.
        $sql_new_collection = "
          SELECT stock_id AS id FROM {stock} INNER JOIN {organism} USING(organism_id) 
          WHERE organism.genus = :genus AND stock.stock_id IN 
            (SELECT stock_id FROM {stockcollection_stock} WHERE stockcollection_id = :collection) 
          LIMIT 1
        ";
        
        // When user starts selecting filters... prepare
        // collection field and suggest relevant to selections.
        $new_collection_options = [0 => $default_text];
        foreach($collection_options as $i => $collection) {
          if ($i > 0) {
            // Skip default select option 0.

            $has_genus = chado_query($sql_new_collection, [':genus' => $genus, ':collection' => $i])
              ->fetchObject();
            
            if ($has_genus && $has_genus > 0) {
              $new_collection_options[ $i ] = $collection;
            }
          }
        }
      }
      
      // Use currently selected collection not the default.
      $default_collection = $q['collection'];
    }
    
    $options = (count($new_collection_options) > 1) ? $new_collection_options : $collection_options;
    $collection_field = $class::$info['filters']['collection'];

    $form['collection'] = array(
      '#type' => 'select',
      '#title' => t($collection_field['title']),
      '#description' => t($collection_field['help']),
      '#options' => $options,
      '#default_value' => $default_collection
    );
  }
}
