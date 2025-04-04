/**
 * @file data_extractor.cpp
 * @brief Class implementation file responsible for exracting the ground-truth, odometry and measurement data from the UTIAS multi-robot localisation dataset.
 * @details The class extracts the data from the groundtruth, measurement, and populates three main 
 * @author Daniel Ingham
 * @date 2025-03-28
 */

#include "../include/data_extractor.h"

/**
 * @brief Default constructor.
 */
DataExtractor::DataExtractor(){

}
/**
 * @brief Constructor that extracts and populates class attributes using the values from the dataset provided.
 * @param[in] path to the dataset folder.
 * @note The dataset extractor constructor only takes one dataset at at time.
 */
DataExtractor::DataExtractor(const std::string& dataset, double sample_period = 0.02){
	setDataSet(dataset);
	syncData(sample_period);
}

/**
 * @brief Extracts data from the barcodes data file: Barcodes.dat.
 * @param[in] path to the dataset folder.
 */
bool DataExtractor::readBarcodes(const std::string& dataset) {
	/* Check that the dataset was specified */
	if ("" == this->dataset_) {
		std::cerr << "Please specify a dataset\n";
		return false;
	}

	std::string filename = dataset + "/Barcodes.dat";
	std::ifstream file(filename);


	if (!file.is_open()) {
		std::cerr << "[ERROR] Unable to open barcodes file:"<< filename << std::endl;
		return false;
	}

	/* Iterate through file line by line.*/
	std::string line;
	int i = 0; 

	while (std::getline(file, line)) {
		/* Ignore file comments. */
		if (line[0] == '#') {
			continue;
		}

		/* Remove whitespaces */ 
		line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
		
		if (i >= TOTAL_BARCODES) {
			std::cerr << "[ERROR] total read barcodes exceeds TOTAL_BARCODES\n";
			return false;
		}

		/* Extract barcodes into barcodes array */
		barcodes_[i++] = std::stoi(line.substr(line.find('\t', 0))) ;
	}

	file.close();

	return true;
}

/**
 * @brief Extracts data from the landmarks data file: Landmark_Groundtruth.dat.
 * @param[in] path to the dataset folder.
 * @note DataExtractor::readBarcodes needs to be called before this function since this function relies on the barcodes it extracted.
 */
bool DataExtractor::readLandmarks(const std::string& dataset) {

	std::string filename = dataset + "/Landmark_Groundtruth.dat";
	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "[ERROR] Unable to open Landmarks file:" << filename << std::endl;
		return false;
	}
	/* Iterate through file line by line.*/
	int i = 0; 
	std::string line;

	while (std::getline(file, line)) {
		/* Ignore file comments. */
		if ('#' == line[0]) {
			continue;
		}

		/* Remove whitespaces */
		line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
		
		if (i >= TOTAL_LANDMARKS) { 
			std::cerr << "[ERROR] total read landmarks exceeds TOTAL_LANDMARKS\n";
			return false;
		}

		/* Set the landmark's ID */
		std::size_t start_index = 0; 
		std::size_t end_index = line.find('\t', 0);
		landmarks_[i].id = std::stoi(line.substr(start_index, end_index));

		/* Ensure that the barcodes have been extracted and set */
		if (barcodes_[landmarks_[i].id - 1] == 0) { 
			std::cerr << "[ERROR] Barcodes not correctly set" << std::endl;
			return false;
		}

		/* Set landmark's barcode */
		landmarks_[i].barcode = barcodes_[landmarks_[i].id - 1] ;
		
		start_index = end_index; 
		end_index = line.find('\t', end_index+1);
		landmarks_[i].x = std::stod(line.substr(start_index, end_index));

		start_index = end_index; 
		end_index = line.find('\t', end_index+1);
		landmarks_[i].y = std::stod(line.substr(start_index, end_index));

		start_index = end_index; 
		end_index = line.find('\t', end_index+1);
		landmarks_[i].x_std_dev = std::stod(line.substr(start_index, end_index));

		start_index = end_index; 
		end_index = line.find('\t', end_index+1);
		landmarks_[i++].y_std_dev = std::stod(line.substr(start_index, end_index));
	}

	file.close();

	return true;
}

/**
 * @brief Extracts data from the groundtruth data file: Robotx_Groundtruth.dat.
 * @param[in] path to the dataset folder.
 */
bool DataExtractor::readGroundTruth(const std::string& dataset, int robot_id) {
	/* Clear all previous elements in the ground truth vector. */
	robots_[robot_id].raw.ground_truth.clear();

	/* Setup file for data extraction */
	std::string filename = dataset + "/Robot" + std::to_string(robot_id + 1) + "_Groundtruth.dat"; 
	std::ifstream file(filename);

	/* Check if the file could be opened */
	if (!file.is_open()) {
		std::cerr << "[ERROR] Unable to open Groundtruth file:" << filename << std::endl;
		return false;
	}

	/* Loop through each line in the file. */
	std::string line;
	while (std::getline(file, line)) {
		/* Ignore Comments */
		if ('#' == line[0]) {
			continue;
		}

		/* Remove whitespaces */
		line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

		/* Extract Data into thier respective variables */
		/* - Time */
		std::size_t start_index = 0;
		std::size_t end_index = line.find('\t', 0);
		double time = std::stod(line.substr(start_index, end_index));

		/* - x-coordinate [m] */
		start_index = end_index;
		end_index = line.find('\t', ++end_index);
		double x_coordinate = std::stod(line.substr(start_index, end_index));

		/* - y-coordinate [m] */
		start_index = end_index; 
		end_index = line.find('\t', ++end_index);
		double y_coordinate = std::stod(line.substr(start_index, end_index));

		/* - Orientaiton [rad] */
		start_index = end_index;
		end_index = line.find('\t', ++end_index);
		double orientation = std::stod(line.substr(start_index, end_index));

		/* Populate robot groundtruth with exracted values. */
		robots_[robot_id].raw.ground_truth.push_back(Groundtruth(time, x_coordinate, y_coordinate, orientation));
	}

	file.close();
	return true;
}

/**
 * @brief Extracts data from the groundtruth data file: Robotx_Odometry.dat.
 * @param[in] path to the dataset folder.
 */
bool DataExtractor::readOdometry(const std::string& dataset, int robot_id) {
	/* Clear all previous elements in the odometry vector. */
	robots_[robot_id].raw.odometry.clear();

	/* Setup file for data extraction */
	std::string filename = dataset + "/Robot" + std::to_string(robot_id + 1) +"_Odometry.dat";
	std::fstream file(filename); 

	/* Check if the file could be opened */
	if (!file.is_open()) {
		std::cerr << "[ERROR] Unable to open Odometry file:" << filename << std::endl;
		return false;
	}

	/* Loop through each line in the file. */
	std::string line;
	while (std::getline(file, line)) {
		/* Ignore Comments */
		if ('#' == line[0]) {
			continue;
		}
		/* Remove Whitespace */
		line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

		/* Extract Data into thier respective variables */
		/* - Time */
		std::size_t start_index = 0;
		std::size_t end_index = line.find('\t', 0); 
		double time = std::stod(line.substr(start_index, end_index));

		/* - Forward Velocity [m/s] */
		start_index = end_index;
		end_index = line.find('\t', ++end_index);
		double forward_velocity = std::stod(line.substr(start_index, end_index));
;
		/* - Angular Velocity [rad/s] */
		start_index = end_index;
		end_index = line.find('\t', ++end_index);
		double angular_velocity = std::stod(line.substr(start_index, end_index));
;
		/* Populate the robot struct with the extracted values. */
		robots_[robot_id].raw.odometry.push_back(Odometry(time, forward_velocity, angular_velocity));
	}

	file.close();
	return true;
}

/**
 * @brief Extracts data from the groundtruth data file: Robotx_Measurement.dat.
 * @param[in] path to the dataset folder.
 */
bool DataExtractor::readMeasurements(const std::string& dataset, int robot_id) {
	/* Clear all previous elements in the measurement vector. */
	robots_[robot_id].raw.measurements.clear();

	/* Setup file for data extraction */
	std::string filename = dataset + "/Robot" + std::to_string(robot_id+1) + "_Measurement.dat";
	std::fstream file(filename);

	/* Check if the file could be opened */
	if (!file.is_open()) {
		std::cerr << "[ERROR] Unable to open Measurement file:" << filename << std::endl;
		return false;
	}

	/* Loop through each line in the file. */
	std::string line;
	while (std::getline(file, line)) {
		/* Ignore Comments */
		if ('#' == line[0]) {
			continue;
		}
		/* Remove Whitespaces */
		line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

		/* Extract Data into thier respective variables */
		/* - Time [s]*/
		std::size_t start_index = 0;
		std::size_t end_index = line.find('\t', 0);
		double time = std::stod(line.substr(start_index, end_index));
		
		/* - Subject (ID) */
		start_index = end_index;
		end_index = line.find('\t', ++end_index);
		int subject = std::stoi(line.substr(start_index, end_index));

		/* - Range [m] */
		start_index = end_index;
		end_index = line.find('\t', ++end_index);
		double range = std::stod(line.substr(start_index, end_index));

		/* - Bearing [rad] */
		start_index = end_index;
		end_index = line.find('\t', ++end_index);
		double bearing = std::stod(line.substr(start_index, end_index));

		/* Check if the current time index falls within 0.05 seconds of a previous time index. */
		auto iterator = std::find_if(robots_[robot_id].raw.measurements.begin(), robots_[robot_id].raw.measurements.end(), [&](const Measurement& index) {
			return index.time >= time - 0.05 && index.time <= time + 0.05;
		});

		/* If the timestamp already exists in the vector of measurements, append the current measurment to the timestamp.*/
		if (iterator != robots_[robot_id].raw.measurements.end()) {
			iterator->subjects.push_back(subject);
			iterator->ranges.push_back(range);
			iterator->bearings.push_back(bearing);
		}
		else {
			robots_[robot_id].raw.measurements.push_back(Measurement(time, subject, range, bearing));
		}
	}
	
	file.close();
	return true;
}

/**
 * @brief Extracts data from the all files in the specified dataset folder.
 * @param[in] path to the dataset folder.
 * @detail The function only checks the existence of the given datset folder and call the other "read" functions for the data extraction. 
 */
void DataExtractor::setDataSet(const std::string& dataset) {
	/* Check if the data set directory exists */
	struct stat sb;
	const char* directory = dataset.c_str();

	if (stat(directory, &sb) == 0) {
		this->dataset_ = dataset;
	}
	else {
		throw std::runtime_error("Dataset file path does not exist"); 
	}
	
	/* Perform data extraction in the directory */
	bool barcodes_correct = readBarcodes(dataset);
	bool landmarks_correct = readLandmarks(dataset);
	bool groundtruth_correct = true;
	bool odometry_correct = true;
	bool measurement_correct = true;

	for (int i = 0; i < TOTAL_ROBOTS; i++) {
		groundtruth_correct &= readGroundTruth(dataset, i);
		odometry_correct &= readOdometry(dataset, i);
		measurement_correct &= readMeasurements(dataset, i);
	}
	bool successful_extraction = barcodes_correct & landmarks_correct & groundtruth_correct & odometry_correct & measurement_correct;

	if (!successful_extraction) {
		throw std::runtime_error("Unable to extract data from dataset");
	}
}

/**
 * @brief Getter for the array of Barcodes.
 * @return Returns an integer pointer to the array of barcodes extracted from the barcodes data file: Barcodes.dat.
 */
int* DataExtractor::getBarcodes() {
	if ("" ==  this->dataset_) {
		throw std::runtime_error("Dataset has not been specified during object instantiation. Please ensure you call void setDataSet(std::string) before attempting to get data."); 
	}
	return barcodes_;
}

/**
 * @brief Getter for the array of Landmarks.
 * @return Returns a pointer to the Landmarks structure member, populated by extracting data form Landmarks.dat.
 */
DataExtractor::Landmark* DataExtractor::getLandmarks() {
	if ("" ==  this->dataset_) {
		throw std::runtime_error("Dataset has not been specified during object instantiation. Please ensure you call void setDataSet(std::string) before attempting to get data.");
	}
	return landmarks_;
}

/**
 * @brief Getter for the array of robots.
 * @return Returns a pointer to the Robot structure member, populated by extracting datefrom Robotx_Groundtruth.dat, Robotx_Odometry.dat, and Robotx_Measurement.dat.
 */
DataExtractor::Robot* DataExtractor::getRobots() {
	if ("" == this->dataset_) {
		throw std::runtime_error("Dataset has not been specified during object instantiation. Please ensure you call void setDataSet(std::string) before attempting to get data.");
	}

	return robots_;
}

bool DataExtractor::setSamplePeriod(double sample_period){
	return false;
}

bool DataExtractor::syncData(double sample_period) {
	setSamplePeriod(sample_period);
	return false;
}
